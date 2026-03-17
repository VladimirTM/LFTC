#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "utils.h"
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        err("Argument invalid");
        exit(EXIT_FAILURE);
    }
    char *source = loadFile(argv[1]);
    Token *tokens = tokenize(source);
    showTokens(tokens);
    return 0;
}
