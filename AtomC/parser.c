#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token *iTk;		   // the iterator in the tokens list
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

bool structDef();
bool varDef();
bool typeBase();
bool arrayDecl();
bool fnDef();
bool fnParam();
bool stm();
bool stmCompound();
bool expr();
bool exprAssign();
bool exprOr();
bool exprAnd();
bool exprEq();
bool exprRel();
bool exprAdd();
bool exprMul();
bool exprCast();
bool exprUnary();
bool exprPostfix();
bool exprPrimary();

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
				for (;;)
				{
					if (varDef())
					{
					}
					else
						break;
				}
				if (consume(RACC))
				{
					if (consume(SEMICOLON))
					{
						return true;
					}
					tkerr("Lipsa ; dupa definitia structurii");
				}
				tkerr("Lipsa } in definitia structurii");
			}
		}
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
			if (arrayDecl())
			{
			}
			if (consume(SEMICOLON))
			{
				return true;
			}
			tkerr("Lipsa ; dupa declaratia de variabila");
		}
	}
	iTk = start;
	return false;
}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase()
{
	if (consume(TYPE_INT))
	{
		return true;
	}
	if (consume(TYPE_DOUBLE))
	{
		return true;
	}
	if (consume(TYPE_CHAR))
	{
		return true;
	}
	if (consume(STRUCT))
	{
		if (consume(ID))
		{
			return true;
		}
	}
	return false;
}

// arrayDecl: LBRACKET INT? RBRACKET

bool arrayDecl()
{
	Token *start = iTk;
	if (consume(LBRACKET))
	{
		if (consume(INT))
		{
		}
		if (consume(RBRACKET))
		{
			return true;
		}
		tkerr("Lispa ] in declaratia de array");
	}
	iTk = start;
	return false;
}

// fnDef: (typeBase | VOID) ID
//				LPAR ( fnParam ( COMMA fnParam)* )? RPAR
//				stmCompound

bool fnDef()
{
	Token *start = iTk;
	if (typeBase() || consume(VOID))
	{
		if (consume(ID))
		{
			if (consume(LPAR))
			{
				if (fnParam())
				{
					while (consume(COMMA))
					{
						if (!fnParam())
						{
							tkerr("Parametru invalid dupa ,");
						}
					}
				}
				if (consume(RPAR))
				{
					if (stmCompound())
					{
						return true;
					}
					tkerr("Lipsa corp functie");
				}
				tkerr("Lipsa ) in definitia functiei");
			}
		}
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
			if (arrayDecl())
			{
			}
			return true;
		}
	}
	iTk = start;
	return false;
}

// stm: stmCompound
//	| IF LPAR expr RPAR stm (ELSE stm)?
//	| WHILE LPAR expr RPAR stm
//	| RETURN expr? SEMICOLON
//	| expr? SEMICOLON

bool stm()
{
	Token *start = iTk;
	if(stmCompound())
	{
		return true;
	}
	if(consume(IF))
	{
		if(consume(LPAR))
		{
			if(expr())
			{
				if(consume(RPAR))
				{
					if(stm())
					{
						if(consume(ELSE))
						{
							if(!stm())
							{
								tkerr("Declaratie invalida dupa else");
							}
						}
						return true;
					}
					tkerr("Declaratie invalida in corpul if");
				}
				tkerr("Lipsa ) dupa conditia if");
			}
			tkerr("Conditie invalida in if");
		}
		tkerr("Lipsa ( dupa if");
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
	{
		return true;
	}
	return false;
}

void parse(Token *tokens)
{
	iTk = tokens;
	if (!unit())
		tkerr("syntax error");
}
