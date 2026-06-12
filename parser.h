#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "lexer.h"
#include "error.h"

/*
 * Grammar (subset of C-like language):
 *
 * program         → declaration* EOF
 * declaration     → func_decl | var_decl
 * func_decl       → type IDENT '(' param_list? ')' block
 * param_list      → param (',' param)*
 * param           → type IDENT
 * var_decl        → type IDENT ('=' expression)? ';'
 * block           → '{' statement* '}'
 * statement       → var_decl
 *                 | if_stmt
 *                 | while_stmt
 *                 | for_stmt
 *                 | return_stmt
 *                 | input_stmt
 *                 | output_stmt
 *                 | expr_stmt
 * if_stmt         → 'if' '(' expression ')' block ('else' block)?
 * while_stmt      → 'while' '(' expression ')' block
 * for_stmt        → 'for' '(' for_init ';' expression ';' for_update ')' block
 * for_init        → var_decl_no_semi | assign_expr | ε
 * for_update      → assign_expr | inc_dec | ε
 * return_stmt     → 'return' expression? ';'
 * input_stmt      → 'input' IDENT ';'
 * output_stmt     → 'output' expression ';'
 * expr_stmt       → expression ';'
 * expression      → or_expr (('='|'+='|'-=') or_expr)?
 * or_expr         → and_expr ('||' and_expr)*
 * and_expr        → equality ('&&' equality)*
 * equality        → relational (('=='|'!=') relational)*
 * relational      → additive (('<'|'>'|'<='|'>=') additive)*
 * additive        → multiplicative (('+'|'-') multiplicative)*
 * multiplicative  → unary (('*'|'/'|'%') unary)*
 * unary           → ('!'|'-') unary | postfix
 * postfix         → primary ('++'|'--')?
 * primary         → INT_LIT | FLOAT_LIT | BOOL_LIT | IDENT | call_expr
 *                 | '(' expression ')'
 * call_expr       → IDENT '(' arg_list? ')'
 * arg_list        → expression (',' expression)*
 * type            → 'int' | 'float' | 'bool' | 'void'
 */

typedef struct {
    Token       *tokens;
    int          count;
    int          pos;
    ErrorHandler *err;
} Parser;

void parser_init  (Parser *p, Lexer *lex, ErrorHandler *err);
void parser_parse (Parser *p);  /* validates structure; reports syntax errors */

#endif /* PARSER_H */
