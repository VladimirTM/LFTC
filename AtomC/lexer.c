#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"

Token *tokens;
Token *lastTk;
int line = 1;

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
	size_t len = end - begin;
	char *text = safeAlloc(len + 1);
	memcpy(text, begin, len);
	text[len] = '\0';
	return text;
}

int getEscapeCharacter(char ch)
{
	switch (ch)
	{
	case 'a':
		return '\a';
	case 'b':
		return '\b';
	case 'f':
		return '\f';
	case 'n':
		return '\n';
	case 'r':
		return '\r';
	case 't':
		return '\t';
	case 'v':
		return '\v';
	case '\\':
		return '\\';
	case '\'':
		return '\'';
	case '"':
		return '"';
	case '0':
		return '\0';
	default:
		err("secventa de escape invalida: \\%c", ch);
	}
	return 0;
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
		case '\r':
			if (pch[1] == '\n')
			{
				pch++;
			}
			// fallthrough
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
				err("caracter invalid: &");
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
				err("caracter invalid: |");
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
		case '\'':
			pch++;
			char ch;
			if (*pch == '\\')
			{
				pch++;
				ch = getEscapeCharacter(*pch);
			}
			else
			{
				ch = *pch;
			}
			pch++;
			if (*pch != '\'')
			{
				err("lipsesc ghilimelele simple de inchidere");
			}
			tk = addTk(CHAR);
			tk->c = ch;
			pch++;
			break;
		case '"':
		{
			size_t bufferSize = 64, length = 0;
			char *buffer = safeAlloc(bufferSize);
			pch++;
			while (*pch != '"')
			{
				if (*pch == '\0')
				{
					err("lipsesc ghilimelele duble de inchidere");
				}
				char ch;
				if (*pch == '\\')
				{
					pch++;
					ch = getEscapeCharacter(*pch);
				}
				else
				{
					ch = *pch;
				}
				if (length + 1 >= bufferSize)
				{
					bufferSize = bufferSize * 2;
					char *tmp = realloc(buffer, bufferSize);
					if (!tmp)
						err("not enough memory");
					buffer = tmp;
				}
				buffer[length++] = ch;
				pch++;
			}
			buffer[length] = '\0';
			pch++;
			tk = addTk(STRING);
			tk->text = buffer;
			break;
		}
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
			else if (isdigit(*pch))
			{
				for (start = pch; isdigit(*pch); pch++)
				{
				}
				int isDouble = 0;
				if (*pch == '.')
				{
					pch++;
					if (isdigit(*pch))
					{
						isDouble = 1;
						for (pch++; isdigit(*pch); pch++)
						{
						}
					}
					else
					{
						err("Cifre lipsa dupa punctul zecimal");
					}
				}
				if (*pch == 'e' || *pch == 'E')
				{
					isDouble = 1;
					pch++;
					if (*pch == '+' || *pch == '-')
					{
						pch++;
					}
					if (!isdigit(*pch))
					{
						err("Cifre lipsa dupa exponent");
					}
					while (isdigit(*pch))
					{
						pch++;
					}
				}
				char *number = extract(start, pch);
				if (isDouble)
				{
					tk = addTk(DOUBLE);
					tk->d = atof(number);
				}
				else
				{
					tk = addTk(INT);
					tk->i = atoi(number);
				}
				free(number);
			}
			else
				err("caracter invalid: %c (%d)", *pch, *pch);
		}
	}
}

void freeTokens(Token *tk)
{
	while (tk)
	{
		Token *next = tk->next;
		if (tk->code == ID || tk->code == STRING)
			free(tk->text);
		free(tk);
		tk = next;
	}
}

void showTokens(const Token *tokens)
{
	static const char *names[] = {
		"ID", "TYPE_CHAR", "TYPE_DOUBLE", "ELSE", "IF", "TYPE_INT",
		"RETURN", "STRUCT", "VOID", "WHILE", "INT", "DOUBLE", "CHAR",
		"STRING", "COMMA", "SEMICOLON", "LPAR", "RPAR", "LBRACKET",
		"RBRACKET", "LACC", "RACC", "END", "ADD", "SUB", "MUL", "DIV",
		"DOT", "AND", "OR", "NOT", "ASSIGN", "EQUAL", "NOTEQ", "LESS",
		"LESSEQ", "GREATER", "GREATEREQ"};

	int currentLine = -1;
	for (const Token *tk = tokens; tk; tk = tk->next)
	{
		if (tk->line != currentLine)
		{
			if (currentLine != -1)
				printf("\n");
			printf("Line %-3d: ", tk->line);
			currentLine = tk->line;
		}

		printf("%s", names[tk->code]);
		if (tk->code == ID || tk->code == STRING)
			printf(":%s", tk->text);
		else if (tk->code == INT)
			printf(":%d", tk->i);
		else if (tk->code == DOUBLE)
			printf(":%.2f", tk->d);
		else if (tk->code == CHAR)
			printf(":%c", tk->c);
		printf(" ");
	}
	printf("\n");
}
