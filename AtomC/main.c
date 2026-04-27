#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "ad.h"
#include "utils.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        err("argument invalid");
        exit(EXIT_FAILURE);
    }
    char *source = loadFile(argv[1]);
    Token *tokens = tokenize(source);
    showTokens(tokens);
    printf("\n");
    fflush(stdout);

    pushDomain();        // create global domain before parsing
    parse(tokens);
    showDomain(symTable, "global");  // print the global symbol table
    dropDomain();        // clean up

    printf("Parsare OK\n");
    free(source);
    freeTokens(tokens);
    return 0;
}