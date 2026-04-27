#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"
#include "ad.h"
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
bool expr();

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
					if (stmCompound(false)) // body reuses the already-pushed domain
					{
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

bool exprPrimary()
{
	Token *start = iTk;
	if (consume(ID))
	{
		if (consume(LPAR))
		{
			if (expr())
			{
				while (consume(COMMA))
				{
					if (!expr())
						tkerr("expresie invalida dupa ,");
				}
			}
			if (!consume(RPAR))
				tkerr("lipseste ) dupa argumentele functiei");
		}
		return true;
	}
	if (consume(INT))
		return true;
	if (consume(DOUBLE))
		return true;
	if (consume(CHAR))
		return true;
	if (consume(STRING))
		return true;
	if (consume(LPAR))
	{
		if (expr())
		{
			if (consume(RPAR))
				return true;
			tkerr("lipseste ) dupa expresie");
		}
	}
	iTk = start;
	return false;
}

bool exprPostfixPrim()
{
	if (consume(LBRACKET))
	{
		if (expr())
		{
			if (!consume(RBRACKET))
				tkerr("lipseste ] dupa indexul array-ului");
			exprPostfixPrim();
			return true;
		}
		tkerr("expresie invalida in indexul array-ului");
	}
	if (consume(DOT))
	{
		if (!consume(ID))
			tkerr("lipseste numele campului dupa .");
		exprPostfixPrim();
		return true;
	}
	return true; // epsilon
}

bool exprPostfix()
{
	Token *start = iTk;
	if (exprPrimary())
	{
		return exprPostfixPrim();
	}
	iTk = start;
	return false;
}

bool exprUnary()
{
	Token *start = iTk;
	const char *unaryOp = NULL;
	if (consume(SUB))
		unaryOp = "-";
	else if (consume(NOT))
		unaryOp = "!";
	if (unaryOp)
	{
		if (exprUnary())
			return true;
		tkerr("expresie invalida dupa operatorul unar '%s'", unaryOp);
	}
	if (exprPostfix())
		return true;
	iTk = start;
	return false;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast()
{
	Token *start = iTk;
	if (consume(LPAR))
	{
		Type t;
		if (typeBase(&t))
		{
			arrayDecl(&t); // optional
			if (consume(RPAR))
			{
				if (exprCast())
					return true;
				tkerr("expresie invalida dupa cast");
			}
			tkerr("lipseste ) dupa tipul conversiei");
		}
	}
	iTk = start;
	if (exprUnary())
		return true;
	return false;
}

bool exprMulPrim()
{
	const char *mulOp = NULL;
	if (consume(MUL))
		mulOp = "*";
	else if (consume(DIV))
		mulOp = "/";
	if (mulOp)
	{
		if (exprCast())
		{
			exprMulPrim();
			return true;
		}
		tkerr("expresie invalida dupa operatorul '%s'", mulOp);
	}
	return true;
}

bool exprMul()
{
	Token *start = iTk;
	if (exprCast())
	{
		return exprMulPrim();
	}
	iTk = start;
	return false;
}

bool exprAddPrim()
{
	const char *addOp = NULL;
	if (consume(ADD))
		addOp = "+";
	else if (consume(SUB))
		addOp = "-";
	if (addOp)
	{
		if (exprMul())
		{
			exprAddPrim();
			return true;
		}
		tkerr("expresie invalida dupa operatorul '%s'", addOp);
	}
	return true;
}

bool exprAdd()
{
	Token *start = iTk;
	if (exprMul())
	{
		return exprAddPrim();
	}
	iTk = start;
	return false;
}

bool exprRelPrim()
{
	const char *relOp = NULL;
	if (consume(LESS))
		relOp = "<";
	else if (consume(LESSEQ))
		relOp = "<=";
	else if (consume(GREATER))
		relOp = ">";
	else if (consume(GREATEREQ))
		relOp = ">=";
	if (relOp)
	{
		if (exprAdd())
		{
			exprRelPrim();
			return true;
		}
		tkerr("expresie invalida dupa operatorul relational '%s'", relOp);
	}
	return true;
}

bool exprRel()
{
	Token *start = iTk;
	if (exprAdd())
	{
		return exprRelPrim();
	}
	iTk = start;
	return false;
}

bool exprEqPrim()
{
	const char *eqOp = NULL;
	if (consume(EQUAL))
		eqOp = "==";
	else if (consume(NOTEQ))
		eqOp = "!=";
	if (eqOp)
	{
		if (exprRel())
		{
			exprEqPrim();
			return true;
		}
		tkerr("expresie invalida dupa operatorul '%s'", eqOp);
	}
	return true;
}

bool exprEq()
{
	Token *start = iTk;
	if (exprRel())
	{
		return exprEqPrim();
	}
	iTk = start;
	return false;
}

bool exprAndPrim()
{
	if (consume(AND))
	{
		if (exprEq())
		{
			exprAndPrim();
			return true;
		}
		tkerr("expresie invalida dupa &&");
	}
	return true;
}

bool exprAnd()
{
	Token *start = iTk;
	if (exprEq())
	{
		return exprAndPrim();
	}
	iTk = start;
	return false;
}

bool exprOrPrim()
{
	if (consume(OR))
	{
		if (exprAnd())
		{
			exprOrPrim();
			return true;
		}
		tkerr("expresie invalida dupa ||");
	}
	return true;
}

bool exprOr()
{
	Token *start = iTk;
	if (exprAnd())
	{
		return exprOrPrim();
	}
	iTk = start;
	return false;
}

bool exprAssign()
{
	Token *start = iTk;
	if (exprUnary())
	{
		if (consume(ASSIGN))
		{
			if (exprAssign())
				return true;
			tkerr("expresie invalida dupa =");
		}
	}
	iTk = start;
	if (exprOr())
		return true;
	return false;
}

bool expr()
{
	return exprAssign();
}

bool stm()
{
	Token *start = iTk;

	if (stmCompound(true))
		return true;

	if (consume(IF))
	{
		if (!consume(LPAR))
			tkerr("lipseste ( dupa if");
		if (!expr())
			tkerr("conditie invalida in if");
		if (!consume(RPAR))
			tkerr("lipseste ) dupa conditia if");
		if (!stm())
			tkerr("instructiune invalida dupa if");
		if (consume(ELSE))
		{
			if (!stm())
				tkerr("instructiune invalida dupa else");
		}
		return true;
	}

	if (consume(WHILE))
	{
		if (!consume(LPAR))
			tkerr("lipseste ( dupa while");
		if (!expr())
			tkerr("conditie invalida in while");
		if (!consume(RPAR))
			tkerr("lipseste ) dupa conditia while");
		if (!stm())
			tkerr("instructiune invalida dupa while");
		return true;
	}

	if (consume(RETURN))
	{
		expr(); // optional
		if (!consume(SEMICOLON))
			tkerr("lipseste ; dupa return");
		return true;
	}

	if (expr())
	{
		if (!consume(SEMICOLON))
			tkerr("lipseste ; dupa expresie");
		return true;
	}
	if (consume(SEMICOLON))
		return true;

	iTk = start;
	return false;
}

void parse(Token *tokens)
{
	iTk = tokens;
	if (!unit())
		tkerr("eroare de sintaxa");
}