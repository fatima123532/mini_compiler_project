#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ─── Token Types ─────────────────────────────────────────────────────────── */
typedef enum {
    /* Literals */
    TOK_INT_LIT, TOK_FLOAT_LIT, TOK_BOOL_LIT,

    /* Identifiers & Keywords */
    TOK_IDENT,
    TOK_INT, TOK_FLOAT, TOK_BOOL, TOK_VOID,
    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR,
    TOK_RETURN, TOK_INPUT, TOK_OUTPUT,
    TOK_TRUE, TOK_FALSE,

    /* Arithmetic Operators */
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,

    /* Relational Operators */
    TOK_EQ, TOK_NEQ, TOK_LT, TOK_GT, TOK_LEQ, TOK_GEQ,

    /* Logical Operators */
    TOK_AND, TOK_OR, TOK_NOT,

    /* Assignment */
    TOK_ASSIGN,
    TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN,

    /* Increment / Decrement */
    TOK_INC, TOK_DEC,

    /* Delimiters */
    TOK_LPAREN, TOK_RPAREN,
    TOK_LBRACE, TOK_RBRACE,
    TOK_SEMICOLON, TOK_COMMA,

    TOK_EOF,
    TOK_UNKNOWN
} TokenType;

/* ─── Token Structure ─────────────────────────────────────────────────────── */
typedef struct {
    TokenType type;
    char      lexeme[256];
    int       line;
} Token;

/* ─── Data Types (for semantic analysis) ─────────────────────────────────── */
typedef enum {
    DT_INT,
    DT_FLOAT,
    DT_BOOL,
    DT_VOID,
    DT_UNKNOWN
} DataType;

/* ─── Utility: token type to string ──────────────────────────────────────── */
static inline const char *token_type_str(TokenType t) {
    switch (t) {
        case TOK_INT_LIT:     return "INT_LIT";
        case TOK_FLOAT_LIT:   return "FLOAT_LIT";
        case TOK_BOOL_LIT:    return "BOOL_LIT";
        case TOK_IDENT:       return "IDENT";
        case TOK_INT:         return "int";
        case TOK_FLOAT:       return "float";
        case TOK_BOOL:        return "bool";
        case TOK_VOID:        return "void";
        case TOK_IF:          return "if";
        case TOK_ELSE:        return "else";
        case TOK_WHILE:       return "while";
        case TOK_FOR:         return "for";
        case TOK_RETURN:      return "return";
        case TOK_INPUT:       return "input";
        case TOK_OUTPUT:      return "output";
        case TOK_TRUE:        return "true";
        case TOK_FALSE:       return "false";
        case TOK_PLUS:        return "+";
        case TOK_MINUS:       return "-";
        case TOK_STAR:        return "*";
        case TOK_SLASH:       return "/";
        case TOK_PERCENT:     return "%";
        case TOK_EQ:          return "==";
        case TOK_NEQ:         return "!=";
        case TOK_LT:          return "<";
        case TOK_GT:          return ">";
        case TOK_LEQ:         return "<=";
        case TOK_GEQ:         return ">=";
        case TOK_AND:         return "&&";
        case TOK_OR:          return "||";
        case TOK_NOT:         return "!";
        case TOK_ASSIGN:      return "=";
        case TOK_PLUS_ASSIGN: return "+=";
        case TOK_MINUS_ASSIGN:return "-=";
        case TOK_INC:         return "++";
        case TOK_DEC:         return "--";
        case TOK_LPAREN:      return "(";
        case TOK_RPAREN:      return ")";
        case TOK_LBRACE:      return "{";
        case TOK_RBRACE:      return "}";
        case TOK_SEMICOLON:   return ";";
        case TOK_COMMA:       return ",";
        case TOK_EOF:         return "EOF";
        default:              return "UNKNOWN";
    }
}

static inline const char *datatype_str(DataType dt) {
    switch (dt) {
        case DT_INT:   return "int";
        case DT_FLOAT: return "float";
        case DT_BOOL:  return "bool";
        case DT_VOID:  return "void";
        default:       return "unknown";
    }
}

#endif /* COMMON_H */
