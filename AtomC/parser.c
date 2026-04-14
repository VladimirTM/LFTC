#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token *iTk;        // the iterator in the tokens list
Token *consumedTk; // the last consumed token

void tkerr(const char *fmt, ...)
{
	fprintf(stderr, "error in line %d: ", iTk->line);
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

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase()
{
	if (consume(TYPE_INT))    return true;
	if (consume(TYPE_DOUBLE)) return true;
	if (consume(TYPE_CHAR))   return true;
	if (consume(STRUCT))
	{
		if (consume(ID)) return true;
		tkerr("lipseste numele structurii dupa struct");
	}
	return false;
}

// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl()
{
	Token *start = iTk;
	if (consume(LBRACKET))
	{
		if(consume(INT)) {}
		if (consume(RBRACKET)) return true;
		tkerr("lipseste ] in declaratia de array");
	}
	iTk = start;
	return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef()
{
	Token *start = iTk;
	if (typeBase())
	{
		if (consume(ID))
		{
			if (arrayDecl()) {}
			if (consume(SEMICOLON)) return true;
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
			if (consume(LACC))
			{
				while (varDef()) {}
				if (consume(RACC))
				{
					if (consume(SEMICOLON)) return true;
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
	if (typeBase())
	{
		if (consume(ID))
		{
			if (arrayDecl()) {}
			return true;
		}
		tkerr("lipseste numele parametrului");
	}
	iTk = start;
	return false;
}

bool stm();
bool expr();

// stmCompound: LACC ( varDef | stm )* RACC
bool stmCompound()
{
	Token *start = iTk;
	if (consume(LACC))
	{
		for (;;)
		{
			if (varDef()) {}
			else if (stm()) {}
			else break;
		}
		if (consume(RACC)) return true;
		tkerr("lipseste } la sfarsitul blocului");
	}
	iTk = start;
	return false;
}

// fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR stmCompound
bool fnDef()
{
	Token *start = iTk;
	bool hasType = false;
	if (typeBase())         hasType = true;
	else if (consume(VOID)) hasType = true;

	if (hasType)
	{
		if (consume(ID))
		{
			if (consume(LPAR))
			{
				if (fnParam())
				{
					while (consume(COMMA))
					{
						if (!fnParam()) tkerr("parametru invalid dupa ,");
					}
				}
				if (consume(RPAR))
				{
					if (stmCompound()) return true;
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
		if (structDef()) {}
		else if (fnDef()) {}
		else if (varDef()) {}
		else break;
	}
	if (consume(END)) return true;
	tkerr("token neasteptat: se astepta un tip, struct, void sau sfarsit de fisier");
	return false;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
//            | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
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
					if (!expr()) tkerr("expresie invalida dupa ,");
				}
			}
			if (!consume(RPAR)) tkerr("lipseste ) dupa argumentele functiei");
		}
		return true;
	}
	if (consume(INT))    return true;
	if (consume(DOUBLE)) return true;
	if (consume(CHAR))   return true;
	if (consume(STRING)) return true;
	if (consume(LPAR))
	{
		if (expr())
		{
			if (consume(RPAR)) return true;
			tkerr("lipseste ) dupa expresie");
		}
	}
	iTk = start;
	return false;
}

// exprPostfix: exprPrimary exprPostfixPrim
// exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim
//                | DOT ID exprPostfixPrim
//                | ε
bool exprPostfixPrim()
{
	if (consume(LBRACKET))
	{
		if (expr())
		{
			if (!consume(RBRACKET)) tkerr("lipseste ] dupa indexul array-ului");
			exprPostfixPrim();
			return true;
		}
		tkerr("expresie invalida in indexul array-ului");
	}
	if (consume(DOT))
	{
		if (!consume(ID)) tkerr("lipseste numele campului dupa .");
		exprPostfixPrim();
		return true;
	}
	return true; // ε
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

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary()
{
	Token *start = iTk;
	if (consume(SUB) || consume(NOT))
	{
		if (exprUnary()) return true;
		tkerr("expresie invalida dupa operator unar");
	}
	if (exprPostfix()) return true;
	iTk = start;
	return false;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast()
{
	Token *start = iTk;
	if (consume(LPAR))
	{
		if (typeBase())
		{
			if (arrayDecl()) {}
			if (consume(RPAR))
			{
				if (exprCast()) return true;
				tkerr("expresie invalida dupa cast");
			}
			tkerr("lipseste ) dupa tipul conversiei");
		}
	}
	iTk = start;
	if (exprUnary()) return true;
	return false;
}

// exprMul: exprCast exprMulPrim
// exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | ε
bool exprMulPrim()
{
	if (consume(MUL) || consume(DIV))
	{
		if (exprCast())
		{
			exprMulPrim();
			return true;
		}
		tkerr("expresie invalida dupa * sau /");
	}
	return true; // ε
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

// exprAdd: exprMul exprAddPrim
// exprAddPrim: ( ADD | SUB ) exprMul exprAddPrim | ε
bool exprAddPrim()
{
	if (consume(ADD) || consume(SUB))
	{
		if (exprMul())
		{
			exprAddPrim();
			return true;
		}
		tkerr("expresie invalida dupa + sau -");
	}
	return true; // ε
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

// exprRel: exprAdd exprRelPrim
// exprRelPrim: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRelPrim | ε
bool exprRelPrim()
{
	if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ))
	{
		if (exprAdd())
		{
			exprRelPrim();
			return true;
		}
		tkerr("expresie invalida dupa operator relational");
	}
	return true; // ε
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

// exprEq: exprRel exprEqPrim
// exprEqPrim: ( EQUAL | NOTEQ ) exprRel exprEqPrim | ε
bool exprEqPrim()
{
	if (consume(EQUAL) || consume(NOTEQ))
	{
		if (exprRel())
		{
			exprEqPrim();
			return true;
		}
		tkerr("expresie invalida dupa == sau !=");
	}
	return true; // ε
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

// exprAnd: exprEq exprAndPrim
// exprAndPrim: AND exprEq exprAndPrim | ε
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
	return true; // ε
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

// exprOr: exprAnd exprOrPrim
// exprOrPrim: OR exprAnd exprOrPrim | ε
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
	return true; // ε
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

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign()
{
	Token *start = iTk;
	if (exprUnary())
	{
		if (consume(ASSIGN))
		{
			if (exprAssign()) return true;
			tkerr("expresie invalida dupa =");
		}
	}
	iTk = start;
	if (exprOr()) return true;
	return false;
}

// expr: exprAssign
bool expr()
{
	return exprAssign();
}

// stm: stmCompound
//    | IF LPAR expr RPAR stm ( ELSE stm )?
//    | WHILE LPAR expr RPAR stm
//    | RETURN expr? SEMICOLON
//    | expr? SEMICOLON
bool stm()
{
	Token *start = iTk;

	if (stmCompound()) return true;

	if (consume(IF))
	{
		if (!consume(LPAR)) tkerr("lipseste ( dupa if");
		if (!expr())        tkerr("conditie invalida in if");
		if (!consume(RPAR)) tkerr("lipseste ) dupa conditia if");
		if (!stm())         tkerr("instructiune invalida dupa if");
		if (consume(ELSE))
		{
			if (!stm()) tkerr("instructiune invalida dupa else");
		}
		return true;
	}

	if (consume(WHILE))
	{
		if (!consume(LPAR)) tkerr("lipseste ( dupa while");
		if (!expr())        tkerr("conditie invalida in while");
		if (!consume(RPAR)) tkerr("lipseste ) dupa conditia while");
		if (!stm())         tkerr("instructiune invalida dupa while");
		return true;
	}

	if (consume(RETURN))
	{
		if(expr()) {}
		if (!consume(SEMICOLON)) tkerr("lipseste ; dupa return");
		return true;
	}

	// expr? SEMICOLON
	if (expr())
	{
		if (!consume(SEMICOLON)) tkerr("lipseste ; dupa expresie");
		return true;
	}
	if (consume(SEMICOLON)) return true;

	iTk = start;
	return false;
}

void parse(Token *tokens)
{
	iTk = tokens;
	if (!unit()) tkerr("eroare de sintaxa");
}