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
    Symbol *symMain=findSymbolInDomain(symTable,"main");
    if(!symMain)err("missing main function");
    Instr *entryCode=NULL;
    addInstr(&entryCode,OP_CALL)->arg.instr=symMain->fn.instr;
    addInstr(&entryCode,OP_HALT);
    run(entryCode);
    dropDomain();        // clean up

    printf("\n\nParsare OK\n");
    free(source);
    freeTokens(tokens);
    return 0;
}