#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "common.h"
#include "lexer.h"
#include "symbol_table.h"
#include "error.h"

/*
 * The semantic analyzer performs a second pass over the token stream,
 * building and querying the symbol table to enforce:
 *   1. Declaration before use
 *   2. No duplicate declarations in the same scope
 *   3. Type checking for assignments and binary operations
 *   4. Return type vs declared function return type
 *   5. Function call arity checking
 *   6. Undeclared variable / function references
 */

typedef struct {
    Token        *tokens;
    int           count;
    int           pos;
    SymbolTable  *sym;
    ErrorHandler *err;
    /* track current function return type for return-stmt checking */
    DataType      cur_func_return_type;
    char          cur_func_name[256];
} Semantic;

void semantic_init    (Semantic *s, Lexer *lex, SymbolTable *sym,
                       ErrorHandler *err);
void semantic_analyze (Semantic *s);

#endif /* SEMANTIC_H */
