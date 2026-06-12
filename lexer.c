#include "lexer.h"

/* ─── Keyword table ───────────────────────────────────────────────────────── */
static struct { const char *word; TokenType type; } keywords[] = {
    {"int",    TOK_INT},
    {"float",  TOK_FLOAT},
    {"bool",   TOK_BOOL},
    {"void",   TOK_VOID},
    {"if",     TOK_IF},
    {"else",   TOK_ELSE},
    {"while",  TOK_WHILE},
    {"for",    TOK_FOR},
    {"return", TOK_RETURN},
    {"input",  TOK_INPUT},
    {"output", TOK_OUTPUT},
    {"true",   TOK_TRUE},
    {"false",  TOK_FALSE},
    {NULL,     TOK_UNKNOWN}
};

/* ─── Helpers ─────────────────────────────────────────────────────────────── */
static char peek(Lexer *lex)          { return lex->source[lex->pos]; }
static char peek_next(Lexer *lex)     { return lex->source[lex->pos + 1]; }
static char advance(Lexer *lex)       { return lex->source[lex->pos++]; }
static int  is_at_end(Lexer *lex)     { return lex->source[lex->pos] == '\0'; }

static void add_token(Lexer *lex, TokenType type, const char *lexeme) {
    if (lex->token_count >= MAX_TOKENS) return;
    Token *t  = &lex->tokens[lex->token_count++];
    t->type   = type;
    t->line   = lex->line;
    strncpy(t->lexeme, lexeme, 255);
    t->lexeme[255] = '\0';
}

static TokenType lookup_keyword(const char *word) {
    for (int i = 0; keywords[i].word != NULL; i++)
        if (strcmp(keywords[i].word, word) == 0)
            return keywords[i].type;
    return TOK_IDENT;
}

/* ─── Skip whitespace and comments ───────────────────────────────────────── */
static void skip_whitespace_comments(Lexer *lex) {
    while (!is_at_end(lex)) {
        char c = peek(lex);

        /* Whitespace */
        if (c == ' ' || c == '\r' || c == '\t') {
            advance(lex);
        } else if (c == '\n') {
            lex->line++;
            advance(lex);
        }
        /* Single-line comment */
        else if (c == '/' && peek_next(lex) == '/') {
            while (!is_at_end(lex) && peek(lex) != '\n')
                advance(lex);
        }
        /* Multi-line comment */
        else if (c == '/' && peek_next(lex) == '*') {
            advance(lex); advance(lex); /* consume '/' '*' */
            while (!is_at_end(lex)) {
                if (peek(lex) == '\n') lex->line++;
                if (peek(lex) == '*' && peek_next(lex) == '/') {
                    advance(lex); advance(lex);
                    break;
                }
                advance(lex);
            }
        } else {
            break;
        }
    }
}

/* ─── Scan next token ─────────────────────────────────────────────────────── */
static void scan_token(Lexer *lex) {
    skip_whitespace_comments(lex);
    if (is_at_end(lex)) { add_token(lex, TOK_EOF, "EOF"); return; }

    char c = advance(lex);
    char buf[256];

    /* ── Identifier / keyword ── */
    if (isalpha(c) || c == '_') {
        int len = 0;
        buf[len++] = c;
        while (!is_at_end(lex) && (isalnum(peek(lex)) || peek(lex) == '_'))
            buf[len++] = advance(lex);
        buf[len] = '\0';
        add_token(lex, lookup_keyword(buf), buf);
        return;
    }

    /* ── Integer / Float literal ── */
    if (isdigit(c)) {
        int len = 0;
        buf[len++] = c;
        int is_float = 0;
        while (!is_at_end(lex) && isdigit(peek(lex)))
            buf[len++] = advance(lex);
        if (!is_at_end(lex) && peek(lex) == '.' && isdigit(peek_next(lex))) {
            is_float = 1;
            buf[len++] = advance(lex); /* '.' */
            while (!is_at_end(lex) && isdigit(peek(lex)))
                buf[len++] = advance(lex);
        }
        buf[len] = '\0';
        add_token(lex, is_float ? TOK_FLOAT_LIT : TOK_INT_LIT, buf);
        return;
    }

    /* ── Two-char operators ── */
    switch (c) {
        case '=':
            if (!is_at_end(lex) && peek(lex) == '=') { advance(lex); add_token(lex, TOK_EQ, "=="); return; }
            add_token(lex, TOK_ASSIGN, "="); return;
        case '!':
            if (!is_at_end(lex) && peek(lex) == '=') { advance(lex); add_token(lex, TOK_NEQ, "!="); return; }
            add_token(lex, TOK_NOT, "!"); return;
        case '<':
            if (!is_at_end(lex) && peek(lex) == '=') { advance(lex); add_token(lex, TOK_LEQ, "<="); return; }
            add_token(lex, TOK_LT, "<"); return;
        case '>':
            if (!is_at_end(lex) && peek(lex) == '=') { advance(lex); add_token(lex, TOK_GEQ, ">="); return; }
            add_token(lex, TOK_GT, ">"); return;
        case '&':
            if (!is_at_end(lex) && peek(lex) == '&') { advance(lex); add_token(lex, TOK_AND, "&&"); return; }
            buf[0] = c; buf[1] = '\0';
            err_report(lex->err, ERR_LEXICAL, lex->line, "Unexpected character '&'");
            return;
        case '|':
            if (!is_at_end(lex) && peek(lex) == '|') { advance(lex); add_token(lex, TOK_OR, "||"); return; }
            buf[0] = c; buf[1] = '\0';
            err_report(lex->err, ERR_LEXICAL, lex->line, "Unexpected character '|'");
            return;
        case '+':
            if (!is_at_end(lex) && peek(lex) == '+') { advance(lex); add_token(lex, TOK_INC, "++"); return; }
            if (!is_at_end(lex) && peek(lex) == '=') { advance(lex); add_token(lex, TOK_PLUS_ASSIGN, "+="); return; }
            add_token(lex, TOK_PLUS, "+"); return;
        case '-':
            if (!is_at_end(lex) && peek(lex) == '-') { advance(lex); add_token(lex, TOK_DEC, "--"); return; }
            if (!is_at_end(lex) && peek(lex) == '=') { advance(lex); add_token(lex, TOK_MINUS_ASSIGN, "-="); return; }
            add_token(lex, TOK_MINUS, "-"); return;
        case '*': add_token(lex, TOK_STAR,      "*"); return;
        case '/': add_token(lex, TOK_SLASH,     "/"); return;
        case '%': add_token(lex, TOK_PERCENT,   "%"); return;
        case '(': add_token(lex, TOK_LPAREN,    "("); return;
        case ')': add_token(lex, TOK_RPAREN,    ")"); return;
        case '{': add_token(lex, TOK_LBRACE,    "{"); return;
        case '}': add_token(lex, TOK_RBRACE,    "}"); return;
        case ';': add_token(lex, TOK_SEMICOLON, ";"); return;
        case ',': add_token(lex, TOK_COMMA,     ","); return;
        default:
            buf[0] = c; buf[1] = '\0';
            err_report(lex->err, ERR_LEXICAL, lex->line,
                       "Unexpected character '%s'", buf);
            add_token(lex, TOK_UNKNOWN, buf);
            return;
    }
}

/* ─── Public API ──────────────────────────────────────────────────────────── */
void lexer_init(Lexer *lex, const char *source, ErrorHandler *err) {
    strncpy(lex->source, source, MAX_SOURCE_LEN - 1);
    lex->source[MAX_SOURCE_LEN - 1] = '\0';
    lex->pos         = 0;
    lex->line        = 1;
    lex->token_count = 0;
    lex->err         = err;
}

void lexer_tokenize(Lexer *lex) {
    while (1) {
        scan_token(lex);
        if (lex->token_count > 0 &&
            lex->tokens[lex->token_count - 1].type == TOK_EOF)
            break;
    }
}

void lexer_print(Lexer *lex) {
    printf("\n┌───────────────────────────────────────────────┐\n");
    printf("│               LEXER TOKEN STREAM              │\n");
    printf("├────────┬──────────────────┬───────────────────┤\n");
    printf("│ %-6s │ %-16s │ %-17s │\n", "Line", "Token Type", "Lexeme");
    printf("├────────┼──────────────────┼───────────────────┤\n");
    for (int i = 0; i < lex->token_count; i++) {
        Token *t = &lex->tokens[i];
        printf("│ %-6d │ %-16s │ %-17s │\n",
               t->line, token_type_str(t->type), t->lexeme);
    }
    printf("└────────┴──────────────────┴───────────────────┘\n");
}
