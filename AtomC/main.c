#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "ad.h"
#include "utils.h"
#include "vm.h"

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
    vmInit();            // initialize the virtual machine
    parse(tokens);
    showDomain(symTable, "global");  // print the global symbol table
    Instr *testCode=genDoubleFTestProgram(); // generate a test program
    run(testCode);       // run the test program on the virtual machine
    dropDomain();        // clean up

    printf("\n\nParsare OK\n");
    free(source);
    freeTokens(tokens);
    return 0;
}