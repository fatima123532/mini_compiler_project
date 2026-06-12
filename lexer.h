#ifndef LEXER_H
#define LEXER_H

#include "common.h"
#include "error.h"

#define MAX_SOURCE_LEN 65536
#define MAX_TOKENS     4096

typedef struct {
    char         source[MAX_SOURCE_LEN];
    int          pos;
    int          line;
    Token        tokens[MAX_TOKENS];
    int          token_count;
    ErrorHandler *err;
} Lexer;

void  lexer_init    (Lexer *lex, const char *source, ErrorHandler *err);
void  lexer_tokenize(Lexer *lex);
void  lexer_print   (Lexer *lex);

#endif /* LEXER_H */
