#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"
#include "ad.h"
#include "at.h"
#include "gc.h"
#include "utils.h"

Token *iTk;			  // the iterator in the tokens list
Token *consumedTk;	  // the last consumed token
Symbol *owner = NULL; // current owner: NULL=global, or the fn/struct being defined

void tkerr(const char *fmt, ...)
{
	fprintf(stderr, "eroare la linia %d: ", iTk->line);
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

bool consume(int code)
{
	if (iTk->code == code)
	{
		consumedTk = iTk;
		iTk = iTk->next;
		return true;
	}
	return false;
}

// typeBase[out Type *t]: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(Type *t)
{
	t->n = -1;
	if (consume(TYPE_INT))
	{
		t->tb = TB_INT;
		return true;
	}
	if (consume(TYPE_DOUBLE))
	{
		t->tb = TB_DOUBLE;
		return true;
	}
	if (consume(TYPE_CHAR))
	{
		t->tb = TB_CHAR;
		return true;
	}
	if (consume(STRUCT))
	{
		if (consume(ID))
		{
			Token *tkName = consumedTk;
			t->tb = TB_STRUCT;
			t->s = findSymbol(tkName->text);
			if (!t->s)
				tkerr("structura nedefinita: %s", tkName->text);
			return true;
		}
		tkerr("lipseste numele structurii dupa struct");
	}
	return false;
}

// arrayDecl[inout Type *t]: LBRACKET ( INT | eps ) RBRACKET
bool arrayDecl(Type *t)
{
	Token *start = iTk;
	if (consume(LBRACKET))
	{
		if (consume(INT))
		{
			Token *tkSize = consumedTk;
			t->n = tkSize->i;
		}
		else
		{
			t->n = 0;
		}
		if (consume(RBRACKET))
			return true;
		tkerr("lipseste ] in declaratia de array");
	}
	iTk = start;
	return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef()
{
	Token *start = iTk;
	Type t;
	if (typeBase(&t))
	{
		if (consume(ID))
		{
			Token *tkName = consumedTk;
			if (arrayDecl(&t))
			{
				if (t.n == 0)
					tkerr("o variabila vector trebuie sa aiba o dimensiune specificata");
			}
			if (consume(SEMICOLON))
			{
				Symbol *var = findSymbolInDomain(symTable, tkName->text);
				if (var)
					tkerr("redefinirea simbolului: %s", tkName->text);
				var = newSymbol(tkName->text, SK_VAR);
				var->type = t;
				var->owner = owner;
				addSymbolToDomain(symTable, var);
				if (owner)
				{
					switch (owner->kind)
					{
					case SK_FN:
						var->varIdx = symbolsLen(owner->fn.locals);
						addSymbolToList(&owner->fn.locals, dupSymbol(var));
						break;
					case SK_STRUCT:
						var->varIdx = typeSize(&owner->type);
						addSymbolToList(&owner->structMembers, dupSymbol(var));
						break;
					}
				}
				else
				{
					var->varMem = safeAlloc(typeSize(&t));
				}
				return true;
			}
			tkerr("lipseste ; dupa declaratia de variabila");
		}
	}
	iTk = start;
	return false;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef()
{
	Token *start = iTk;
	if (consume(STRUCT))
	{
		if (consume(ID))
		{
			Token *tkName = consumedTk;
			if (consume(LACC))
			{
				Symbol *s = findSymbolInDomain(symTable, tkName->text);
				if (s)
					tkerr("redefinirea simbolului: %s", tkName->text);
				s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
				s->type.tb = TB_STRUCT;
				s->type.s = s;
				s->type.n = -1;
				pushDomain();
				owner = s;

				while (varDef())
				{
				}

				if (consume(RACC))
				{
					if (consume(SEMICOLON))
					{
						owner = NULL;
						dropDomain();
						return true;
					}
					tkerr("lipseste ; dupa }");
				}
				tkerr("lipseste } la sfarsitul structurii");
			}
			iTk = start;
			return false;
		}
		tkerr("lipseste numele structurii dupa struct");
	}
	iTk = start;
	return false;
}

// fnParam: typeBase ID arrayDecl?
bool fnParam()
{
	Token *start = iTk;
	Type t;
	if (typeBase(&t))
	{
		if (consume(ID))
		{
			Token *tkName = consumedTk;
			if (arrayDecl(&t))
			{
				t.n = 0; // array params lose their specified dimension
			}
			Symbol *param = findSymbolInDomain(symTable, tkName->text);
			if (param)
				tkerr("redefinirea simbolului: %s", tkName->text);
			param = newSymbol(tkName->text, SK_PARAM);
			param->type = t;
			param->owner = owner;
			param->paramIdx = symbolsLen(owner->fn.params);
			addSymbolToDomain(symTable, param);
			addSymbolToList(&owner->fn.params, dupSymbol(param));
			return true;
		}
		tkerr("lipseste numele parametrului");
	}
	iTk = start;
	return false;
}

bool stm();
bool expr(Ret *r);

// stmCompound[in bool newDomain]: LACC ( varDef | stm )* RACC
bool stmCompound(bool newDomain)
{
	Token *start = iTk;
	if (consume(LACC))
	{
		if (newDomain)
			pushDomain();
		for (;;)
		{
			if (varDef())
			{
			}
			else if (stm())
			{
			}
			else
				break;
		}
		if (consume(RACC))
		{
			if (newDomain)
				dropDomain();
			return true;
		}
		tkerr("lipseste } la sfarsitul blocului");
	}
	iTk = start;
	return false;
}

// fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR stmCompound
bool fnDef()
{
	Token *start = iTk;
	Type t;
	bool hasType = false;
	if (typeBase(&t))
	{
		hasType = true;
	}
	else if (consume(VOID))
	{
		t.tb = TB_VOID;
		t.n = -1;
		hasType = true;
	}

	if (hasType)
	{
		if (consume(ID))
		{
			Token *tkName = consumedTk;
			if (consume(LPAR))
			{
				Symbol *fn = findSymbolInDomain(symTable, tkName->text);
				if (fn)
					tkerr("redefinirea simbolului: %s", tkName->text);
				fn = newSymbol(tkName->text, SK_FN);
				fn->type = t;
				addSymbolToDomain(symTable, fn);
				owner = fn;
				pushDomain(); // domain for params + locals

				if (fnParam())
				{
					while (consume(COMMA))
					{
						if (!fnParam())
							tkerr("parametru invalid sau lipseste dupa ,");
					}
				}
				if (consume(RPAR))
				{
					addInstr(&fn->fn.instr,OP_ENTER);
					if (stmCompound(false)) // body reuses the already-pushed domain
					{
						fn->fn.instr->arg.i=symbolsLen(fn->fn.locals);
						if(fn->type.tb==TB_VOID)
							addInstrWithInt(&fn->fn.instr,OP_RET_VOID,symbolsLen(fn->fn.params));
						dropDomain();
						owner = NULL;
						return true;
					}
					tkerr("lipseste corpul functiei");
				}
				tkerr("lipseste ) dupa parametrii functiei");
			}
		}
	}
	iTk = start;
	return false;
}

// unit: ( structDef | fnDef | varDef )* END
bool unit()
{
	for (;;)
	{
		if (structDef())
		{
		}
		else if (fnDef())
		{
		}
		else if (varDef())
		{
		}
		else
			break;
	}
	if (consume(END))
		return true;
	tkerr("token neasteptat: se astepta un tip, struct, void sau sfarsit de fisier");
	return false;
}

bool exprPrimary(Ret *r)
{
	Token *start = iTk;
	Instr *startInstr=owner?lastInstr(owner->fn.instr):NULL;
	if (consume(ID))
	{
		Token *tkName = consumedTk;
		Symbol *s = findSymbol(tkName->text);
		if (!s)
			tkerr("identificator nedefinit: %s", tkName->text);
		if (consume(LPAR))
		{
			if (s->kind != SK_FN)
				tkerr("doar o functie poate fi apelata");
			Ret rArg;
			Symbol *param = s->fn.params;
			if (expr(&rArg))
			{
				if (!param)
					tkerr("prea multe argumente in apelul functiei");
				if (!convTo(&rArg.type, &param->type))
					tkerr("in apel, tipul argumentului nu poate fi convertit la tipul parametrului");
				addRVal(&owner->fn.instr,rArg.lval,&rArg.type);
				insertConvIfNeeded(lastInstr(owner->fn.instr),&rArg.type,&param->type);
				param = param->next;
				while (consume(COMMA))
				{
					if (!expr(&rArg))
						tkerr("expresie invalida dupa ,");
					if (!param)
						tkerr("prea multe argumente in apelul functiei");
					if (!convTo(&rArg.type, &param->type))
						tkerr("in apel, tipul argumentului nu poate fi convertit la tipul parametrului");
					addRVal(&owner->fn.instr,rArg.lval,&rArg.type);
					insertConvIfNeeded(lastInstr(owner->fn.instr),&rArg.type,&param->type);
					param = param->next;
				}
			}
			if (!consume(RPAR))
				tkerr("lipseste ) dupa argumentele functiei");
			if (param)
				tkerr("prea putine argumente in apelul functiei");
			if(s->fn.extFnPtr){
				addInstr(&owner->fn.instr,OP_CALL_EXT)->arg.extFnPtr=s->fn.extFnPtr;
				}else{
				addInstr(&owner->fn.instr,OP_CALL)->arg.instr=s->fn.instr;
				}
			*r = (Ret){s->type, false, true};
		}
		else
		{
			if (s->kind == SK_FN)
				tkerr("o functie poate fi doar apelata");
			if(s->kind==SK_VAR){
				if(s->owner==NULL){
					addInstr(&owner->fn.instr,OP_ADDR)->arg.p=s->varMem;
					}else{
					switch(s->type.tb){
						case TB_INT:addInstrWithInt(&owner->fn.instr,OP_FPADDR_I,s->varIdx+1);break;
						case TB_DOUBLE:addInstrWithInt(&owner->fn.instr,OP_FPADDR_F,s->varIdx+1);break;
						}
					}
				}
			if(s->kind==SK_PARAM){
				switch(s->type.tb){
					case TB_INT:
						addInstrWithInt(&owner->fn.instr,OP_FPADDR_I,s->paramIdx-symbolsLen(s->owner->fn.params)-1);
						break;
					case TB_DOUBLE:
						addInstrWithInt(&owner->fn.instr,OP_FPADDR_F,s->paramIdx-symbolsLen(s->owner->fn.params)-1);
						break;
					}
				}
			*r = (Ret){s->type, true, s->type.n >= 0};
		}
		return true;
	}
	if (consume(INT))
	{
		if(owner)addInstrWithInt(&owner->fn.instr,OP_PUSH_I,consumedTk->i);
		*r = (Ret){{TB_INT, NULL, -1}, false, true};
		return true;
	}
	if (consume(DOUBLE))
	{
		if(owner)addInstrWithDouble(&owner->fn.instr,OP_PUSH_F,consumedTk->d);
		*r = (Ret){{TB_DOUBLE, NULL, -1}, false, true};
		return true;
	}
	if (consume(CHAR))
	{
		*r = (Ret){{TB_CHAR, NULL, -1}, false, true};
		return true;
	}
	if (consume(STRING))
	{
		*r = (Ret){{TB_CHAR, NULL, 0}, false, true};
		return true;
	}
	if (consume(LPAR))
	{
		if (expr(r))
		{
			if (consume(RPAR))
				return true;
			tkerr("lipseste ) dupa expresie");
		}
	}
	iTk = start;
	if(owner)delInstrAfter(startInstr);
	return false;
}

bool exprPostfixPrim(Ret *r)
{
	if (consume(LBRACKET))
	{
		Ret idx;
		if (expr(&idx))
		{
			if (!consume(RBRACKET))
				tkerr("lipseste ] dupa indexul array-ului");
			if (r->type.n < 0)
				tkerr("doar un vector poate fi indexat");
			Type tInt = {TB_INT, NULL, -1};
			if (!convTo(&idx.type, &tInt))
				tkerr("indexul nu este convertibil la int");
			r->type.n = -1;
			r->lval = true;
			r->ct = false;
			return exprPostfixPrim(r);
		}
		tkerr("expresie invalida in indexul array-ului");
	}
	if (consume(DOT))
	{
		if (!consume(ID))
			tkerr("lipseste numele campului dupa .");
		Token *tkName = consumedTk;
		if (r->type.tb != TB_STRUCT)
			tkerr("un camp poate fi selectat doar dintr-o structura");
		Symbol *s = findSymbolInList(r->type.s->structMembers, tkName->text);
		if (!s)
			tkerr("structura %s nu are campul %s", r->type.s->name, tkName->text);
		*r = (Ret){s->type, true, s->type.n >= 0};
		return exprPostfixPrim(r);
	}
	return true; // epsilon
}

bool exprPostfix(Ret *r)
{
	Token *start = iTk;
	if (exprPrimary(r))
	{
		return exprPostfixPrim(r);
	}
	iTk = start;
	return false;
}

bool exprUnary(Ret *r)
{
	Token *start = iTk;
	if (consume(SUB) || consume(NOT))
	{
		if (exprUnary(r))
		{
			if (!canBeScalar(r))
				tkerr("operatorul unar - sau ! trebuie sa aiba un operand scalar");
			r->lval = false;
			r->ct = true;
			return true;
		}
		tkerr("expresie invalida dupa operatorul unar");
	}
	if (exprPostfix(r))
		return true;
	iTk = start;
	return false;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast(Ret *r)
{
	Token *start = iTk;
	Instr *startInstr=owner?lastInstr(owner->fn.instr):NULL;
	if (consume(LPAR))
	{
		Type t;
		if (typeBase(&t))
		{
			arrayDecl(&t); // optional
			if (consume(RPAR))
			{
				Ret op;
				if (exprCast(&op))
				{
					if (t.tb == TB_STRUCT)
						tkerr("nu se poate converti la un tip structura");
					if (op.type.tb == TB_STRUCT)
						tkerr("nu se poate converti o structura");
					if (op.type.n >= 0 && t.n < 0)
						tkerr("un vector poate fi convertit doar la alt vector");
					if (op.type.n < 0 && t.n >= 0)
						tkerr("un scalar poate fi convertit doar la alt scalar");
					*r = (Ret){t, false, true};
					return true;
				}
				tkerr("expresie invalida dupa cast");
			}
			tkerr("lipseste ) dupa tipul conversiei");
		}
	}
	iTk = start;
	if(owner)delInstrAfter(startInstr);
	if (exprUnary(r))
		return true;
	return false;
}

bool exprMulPrim(Ret *r)
{
	int op_code=0;
	if (consume(MUL))
		op_code=MUL;
	else if (consume(DIV))
		op_code=DIV;
	if (op_code)
	{
		Instr *lastLeft=lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr,r->lval,&r->type);
		Ret right;
		if (exprCast(&right))
		{
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("tip invalid de operand pentru * sau /");
			addRVal(&owner->fn.instr,right.lval,&right.type);
			insertConvIfNeeded(lastLeft,&r->type,&tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr),&right.type,&tDst);
			switch(op_code){
				case MUL:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_MUL_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_MUL_F);break;
						}
					break;
				case DIV:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_DIV_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_DIV_F);break;
						}
					break;
				}
			*r = (Ret){tDst, false, true};
			return exprMulPrim(r);
		}
		tkerr("expresie invalida dupa operatorul de inmultire/impartire");
	}
	return true;
}

bool exprMul(Ret *r)
{
	Token *start = iTk;
	if (exprCast(r))
	{
		return exprMulPrim(r);
	}
	iTk = start;
	return false;
}

bool exprAddPrim(Ret *r)
{
	int op_code=0;
	if (consume(ADD))
		op_code=ADD;
	else if (consume(SUB))
		op_code=SUB;
	if (op_code)
	{
		Instr *lastLeft=lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr,r->lval,&r->type);
		Ret right;
		if (exprMul(&right))
		{
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("tip invalid de operand pentru + sau -");
			addRVal(&owner->fn.instr,right.lval,&right.type);
			insertConvIfNeeded(lastLeft,&r->type,&tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr),&right.type,&tDst);
			switch(op_code){
				case ADD:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_ADD_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_ADD_F);break;
						}
					break;
				case SUB:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_SUB_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_SUB_F);break;
						}
					break;
				}
			*r = (Ret){tDst, false, true};
			return exprAddPrim(r);
		}
		tkerr("expresie invalida dupa operatorul de adunare/scadere");
	}
	return true;
}

bool exprAdd(Ret *r)
{
	Token *start = iTk;
	if (exprMul(r))
	{
		return exprAddPrim(r);
	}
	iTk = start;
	return false;
}

bool exprRelPrim(Ret *r)
{
	int op_code=0;
	if (consume(LESS))
		op_code=LESS;
	else if (consume(LESSEQ))
		op_code=LESSEQ;
	else if (consume(GREATER))
		op_code=GREATER;
	else if (consume(GREATEREQ))
		op_code=GREATEREQ;
	if (op_code)
	{
		Instr *lastLeft=lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr,r->lval,&r->type);
		Ret right;
		if (exprAdd(&right))
		{
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("tip invalid de operand pentru <, <=, >, >=");
			addRVal(&owner->fn.instr,right.lval,&right.type);
			insertConvIfNeeded(lastLeft,&r->type,&tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr),&right.type,&tDst);
			switch(op_code){
				case LESS:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_LESS_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_LESS_F);break;
						}
					break;
				}
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			return exprRelPrim(r);
		}
		tkerr("expresie invalida dupa operatorul relational");
	}
	return true;
}

bool exprRel(Ret *r)
{
	Token *start = iTk;
	if (exprAdd(r))
	{
		return exprRelPrim(r);
	}
	iTk = start;
	return false;
}

bool exprEqPrim(Ret *r)
{
	const char *eqOp = NULL;
	if (consume(EQUAL))
		eqOp = "==";
	else if (consume(NOTEQ))
		eqOp = "!=";
	if (eqOp)
	{
		Ret right;
		if (exprRel(&right))
		{
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("tip invalid de operand pentru == sau !=");
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			return exprEqPrim(r);
		}
		tkerr("expresie invalida dupa operatorul '%s'", eqOp);
	}
	return true;
}

bool exprEq(Ret *r)
{
	Token *start = iTk;
	if (exprRel(r))
	{
		return exprEqPrim(r);
	}
	iTk = start;
	return false;
}

bool exprAndPrim(Ret *r)
{
	if (consume(AND))
	{
		Ret right;
		if (exprEq(&right))
		{
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("tip invalid de operand pentru &&");
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			return exprAndPrim(r);
		}
		tkerr("expresie invalida dupa &&");
	}
	return true;
}

bool exprAnd(Ret *r)
{
	Token *start = iTk;
	if (exprEq(r))
	{
		return exprAndPrim(r);
	}
	iTk = start;
	return false;
}

bool exprOrPrim(Ret *r)
{
	if (consume(OR))
	{
		Ret right;
		if (exprAnd(&right))
		{
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("tip invalid de operand pentru ||");
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			return exprOrPrim(r);
		}
		tkerr("expresie invalida dupa ||");
	}
	return true;
}

bool exprOr(Ret *r)
{
	Token *start = iTk;
	if (exprAnd(r))
	{
		return exprOrPrim(r);
	}
	iTk = start;
	return false;
}

bool exprAssign(Ret *r)
{
	Token *start = iTk;
	Instr *startInstr=owner?lastInstr(owner->fn.instr):NULL;
	Ret rDst;
	if (exprUnary(&rDst))
	{
		if (consume(ASSIGN))
		{
			if (exprAssign(r))
			{
				if (!rDst.lval)
					tkerr("destinatia atribuirii trebuie sa fie o valoare stanga");
				if (rDst.ct)
					tkerr("destinatia atribuirii nu poate fi constanta");
				if (!canBeScalar(&rDst))
					tkerr("destinatia atribuirii trebuie sa fie scalara");
				if (!canBeScalar(r))
					tkerr("sursa atribuirii trebuie sa fie scalara");
				if (!convTo(&r->type, &rDst.type))
					tkerr("sursa atribuirii nu poate fi convertita la destinatie");
				addRVal(&owner->fn.instr,r->lval,&r->type);
				insertConvIfNeeded(lastInstr(owner->fn.instr),&r->type,&rDst.type);
				switch(rDst.type.tb){
					case TB_INT:addInstr(&owner->fn.instr,OP_STORE_I);break;
					case TB_DOUBLE:addInstr(&owner->fn.instr,OP_STORE_F);break;
					}
				r->lval = false;
				r->ct = true;
				return true;
			}
			tkerr("expresie invalida dupa =");
		}
	}
	iTk = start;
	if(owner)delInstrAfter(startInstr);
	if (exprOr(r))
		return true;
	return false;
}

bool expr(Ret *r)
{
	return exprAssign(r);
}

bool stm()
{
	Token *start = iTk;
	Instr *startInstr=owner?lastInstr(owner->fn.instr):NULL;

	if (stmCompound(true))
		return true;

	if (consume(IF))
	{
		if (!consume(LPAR))
			tkerr("lipseste ( dupa if");
		Ret rCond;
		if (!expr(&rCond))
			tkerr("conditie invalida in if");
		if (!canBeScalar(&rCond))
			tkerr("conditia if trebuie sa fie o valoare scalara");
		if (!consume(RPAR))
			tkerr("lipseste ) dupa conditia if");
		addRVal(&owner->fn.instr,rCond.lval,&rCond.type);
		Type intType={TB_INT,NULL,-1};
		insertConvIfNeeded(lastInstr(owner->fn.instr),&rCond.type,&intType);
		Instr *ifJF=addInstr(&owner->fn.instr,OP_JF);
		if (!stm())
			tkerr("instructiune invalida dupa if");
		if (consume(ELSE))
		{
			Instr *ifJMP=addInstr(&owner->fn.instr,OP_JMP);
			ifJF->arg.instr=addInstr(&owner->fn.instr,OP_NOP);
			if (!stm())
				tkerr("instructiune invalida dupa else");
			ifJMP->arg.instr=addInstr(&owner->fn.instr,OP_NOP);
		}
		else
		{
			ifJF->arg.instr=addInstr(&owner->fn.instr,OP_NOP);
		}
		return true;
	}

	if (consume(WHILE))
	{
		Instr *beforeWhileCond=lastInstr(owner->fn.instr);
		if (!consume(LPAR))
			tkerr("lipseste ( dupa while");
		Ret rCond;
		if (!expr(&rCond))
			tkerr("conditie invalida in while");
		if (!canBeScalar(&rCond))
			tkerr("conditia while trebuie sa fie o valoare scalara");
		if (!consume(RPAR))
			tkerr("lipseste ) dupa conditia while");
		addRVal(&owner->fn.instr,rCond.lval,&rCond.type);
		Type intType={TB_INT,NULL,-1};
		insertConvIfNeeded(lastInstr(owner->fn.instr),&rCond.type,&intType);
		Instr *whileJF=addInstr(&owner->fn.instr,OP_JF);
		if (!stm())
			tkerr("instructiune invalida dupa while");
		addInstr(&owner->fn.instr,OP_JMP)->arg.instr=beforeWhileCond->next;
		whileJF->arg.instr=addInstr(&owner->fn.instr,OP_NOP);
		return true;
	}

	if (consume(RETURN))
	{
		Ret rExpr;
		if (expr(&rExpr))
		{
			if (owner->type.tb == TB_VOID)
				tkerr("o functie void nu poate returna o valoare");
			if (!canBeScalar(&rExpr))
				tkerr("valoarea returnata trebuie sa fie scalara");
			if (!convTo(&rExpr.type, &owner->type))
				tkerr("tipul expresiei returnate nu poate fi convertit la tipul returnat de functie");
			addRVal(&owner->fn.instr,rExpr.lval,&rExpr.type);
			insertConvIfNeeded(lastInstr(owner->fn.instr),&rExpr.type,&owner->type);
			addInstrWithInt(&owner->fn.instr,OP_RET,symbolsLen(owner->fn.params));
		}
		else
		{
			if (owner->type.tb != TB_VOID)
				tkerr("o functie non-void trebuie sa returneze o valoare");
			addInstr(&owner->fn.instr,OP_RET_VOID);
		}
		if (!consume(SEMICOLON))
			tkerr("lipseste ; dupa return");
		return true;
	}

	Ret rExpr;
	if (expr(&rExpr))
	{
		if(rExpr.type.tb!=TB_VOID)addInstr(&owner->fn.instr,OP_DROP);
		if (!consume(SEMICOLON))
			tkerr("lipseste ; dupa expresie");
		return true;
	}
	if (consume(SEMICOLON))
		return true;

	iTk = start;
	if(owner)delInstrAfter(startInstr);
	return false;
}

void parse(Token *tokens)
{
	iTk = tokens;
	if (!unit())
		tkerr("eroare de sintaxa");
}
