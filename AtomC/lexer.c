#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"

Token *tokens; // single linked list of tokens
Token *lastTk; // the last token in list

int line = 1; // the current line in the input file

// adds a token to the end of the tokens list and returns it
// sets its code and line
Token *addTk(int code)
{
	Token *tk = safeAlloc(sizeof(Token));
	tk->code = code;
	tk->line = line;
	tk->next = NULL;
	if (lastTk)
	{
		lastTk->next = tk;
	}
	else
	{
		tokens = tk;
	}
	lastTk = tk;
	return tk;
}

char *extract(const char *begin, const char *end)
{
	int len = end - begin;
	char *text = safeAlloc(len + 1);
	memcpy(text, begin, len);
	text[len] = '\0';
	return text;
}

Token *tokenize(const char *pch)
{
	const char *start;
	Token *tk;
	for (;;)
	{
		switch (*pch)
		{
		case ' ':
		case '\t':
			pch++;
			break;
		case '\r': // handles different kinds of newlines (Windows: \r\n, Linux: \n, MacOS, OS X: \r or \n)
			if (pch[1] == '\n')
				pch++;
			// fallthrough to \n
		case '\n':
			line++;
			pch++;
			break;
		case '\0':
			addTk(END);
			return tokens;
		case ',':
			addTk(COMMA);
			pch++;
			break;
		case ';':
			addTk(SEMICOLON);
			pch++;
			break;
		case '(':
			addTk(LPAR);
			pch++;
			break;
		case ')':
			addTk(RPAR);
			pch++;
			break;
		case '[':
			addTk(LBRACKET);
			pch++;
			break;
		case ']':
			addTk(RBRACKET);
			pch++;
			break;
		case '{':
			addTk(LACC);
			pch++;
			break;
		case '}':
			addTk(RACC);
			pch++;
			break;
		case '+':
			addTk(ADD);
			pch++;
			break;
		case '-':
			addTk(SUB);
			pch++;
			break;
		case '*':
			addTk(MUL);
			pch++;
			break;
		case '.':
			addTk(DOT);
			pch++;
			break;
		case '/':
			if (pch[1] == '/')
			{
				for (pch += 2; *pch && *pch != '\n' && *pch != '\r'; pch++)
				{
				}
			}
			else
			{
				addTk(DIV);
				pch++;
			}
			break;
		case '&':
			if (pch[1] == '&')
			{
				addTk(AND);
				pch += 2;
			}
			else
			{
				err("invalid char: %c (%d)", *pch, *pch);
			}
			break;
		case '|':
			if (pch[1] == '|')
			{
				addTk(OR);
				pch += 2;
			}
			else
			{
				err("invalid char: %c (%d)", *pch, *pch);
			}
			break;
		case '!':
			if (pch[1] == '=')
			{
				addTk(NOTEQ);
				pch += 2;
			}
			else
			{
				addTk(NOT);
				pch++;
			}
			break;
		case '=':
			if (pch[1] == '=')
			{
				addTk(EQUAL);
				pch += 2;
			}
			else
			{
				addTk(ASSIGN);
				pch++;
			}
			break;
		case '<':
			if (pch[1] == '=')
			{
				addTk(LESSEQ);
				pch += 2;
			}
			else
			{
				addTk(LESS);
				pch++;
			}
			break;
		case '>':
			if (pch[1] == '=')
			{
				addTk(GREATEREQ);
				pch += 2;
			}
			else
			{
				addTk(GREATER);
				pch++;
			}
			break;
		default:
			if (isalpha(*pch) || *pch == '_')
			{
				for (start = pch++; isalnum(*pch) || *pch == '_'; pch++)
				{
				}
				char *text = extract(start, pch);
				if (strcmp(text, "char") == 0)
				{
					free(text);
					addTk(TYPE_CHAR);
				}
				else if (strcmp(text, "double") == 0)
				{
					free(text);
					addTk(TYPE_DOUBLE);
				}
				else if (strcmp(text, "else") == 0)
				{
					free(text);
					addTk(ELSE);
				}
				else if (strcmp(text, "if") == 0)
				{
					free(text);
					addTk(IF);
				}
				else if (strcmp(text, "int") == 0)
				{
					free(text);
					addTk(TYPE_INT);
				}
				else if (strcmp(text, "return") == 0)
				{
					free(text);
					addTk(RETURN);
				}
				else if (strcmp(text, "struct") == 0)
				{
					free(text);
					addTk(STRUCT);
				}
				else if (strcmp(text, "void") == 0)
				{
					free(text);
					addTk(VOID);
				}
				else if (strcmp(text, "while") == 0)
				{
					free(text);
					addTk(WHILE);
				}
				else
				{
					tk = addTk(ID);
					tk->text = text;
				}
			}
			else
				err("invalid char: %c (%d)", *pch, *pch);
		}
	}
}

void showTokens(const Token *tokens)
{
	static const char *names[] = {
		"ID",
		"TYPE_CHAR",
		"TYPE_DOUBLE",
		"ELSE",
		"IF",
		"TYPE_INT",
		"RETURN",
		"STRUCT",
		"VOID",
		"WHILE",
		"INT",
		"DOUBLE",
		"CHAR",
		"STRING",
		"COMMA",
		"SEMICOLON",
		"LPAR",
		"RPAR",
		"LBRACKET",
		"RBRACKET",
		"LACC",
		"RACC",
		"END",
		"ADD",
		"SUB",
		"MUL",
		"DIV",
		"DOT",
		"AND",
		"OR",
		"NOT",
		"ASSIGN",
		"EQUAL",
		"NOTEQ",
		"LESS", "LESSEQ", "GREATER", "GREATEREQ"};
	for (const Token *tk = tokens; tk; tk = tk->next)
	{
		printf("%d\t%s\n", tk->code, names[tk->code]);
	}
}
