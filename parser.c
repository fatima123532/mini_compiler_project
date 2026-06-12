#include "parser.h"

/* ─── Internal helpers ────────────────────────────────────────────────────── */
static Token *cur(Parser *p)  { return &p->tokens[p->pos]; }
static Token *prev(Parser *p) { return &p->tokens[p->pos - 1]; }

static Token *advance_p(Parser *p) {
    if (p->pos < p->count - 1) p->pos++;
    return prev(p);
}

static int check(Parser *p, TokenType t) { return cur(p)->type == t; }

static int match(Parser *p, TokenType t) {
    if (check(p, t)) { advance_p(p); return 1; }
    return 0;
}

static int is_type_token(TokenType t) {
    return t == TOK_INT || t == TOK_FLOAT || t == TOK_BOOL || t == TOK_VOID;
}

/* Error recovery: skip to the next ';' or '}' */
static void synchronize(Parser *p) {
    while (!check(p, TOK_EOF)) {
        if (prev(p)->type == TOK_SEMICOLON) return;
        switch (cur(p)->type) {
            case TOK_INT: case TOK_FLOAT: case TOK_BOOL: case TOK_VOID:
            case TOK_IF: case TOK_WHILE: case TOK_FOR: case TOK_RETURN:
            case TOK_INPUT: case TOK_OUTPUT:
                return;
            default: break;
        }
        advance_p(p);
    }
}

static Token *consume(Parser *p, TokenType t, const char *msg) {
    if (check(p, t)) return advance_p(p);
    err_report(p->err, ERR_SYNTAX, cur(p)->line,
               "%s (got '%s')", msg, cur(p)->lexeme);
    synchronize(p);
    return NULL;
}

/* Forward declarations */
static void parse_statement(Parser *p);
static void parse_block(Parser *p);
static void parse_expression(Parser *p);

/* ─── Expression parsing (recursive descent) ─────────────────────────────── */
static void parse_arg_list(Parser *p) {
    if (check(p, TOK_RPAREN)) return;
    parse_expression(p);
    while (match(p, TOK_COMMA))
        parse_expression(p);
}

static void parse_primary(Parser *p) {
    if (match(p, TOK_INT_LIT))   return;
    if (match(p, TOK_FLOAT_LIT)) return;
    if (match(p, TOK_TRUE))      return;
    if (match(p, TOK_FALSE))     return;

    if (check(p, TOK_IDENT)) {
        advance_p(p);
        /* function call */
        if (match(p, TOK_LPAREN)) {
            parse_arg_list(p);
            consume(p, TOK_RPAREN, "Expected ')' after function arguments");
        }
        return;
    }

    if (match(p, TOK_LPAREN)) {
        parse_expression(p);
        consume(p, TOK_RPAREN, "Expected ')' after grouped expression");
        return;
    }

    err_report(p->err, ERR_SYNTAX, cur(p)->line,
               "Expected expression, got '%s'", cur(p)->lexeme);
    advance_p(p); /* consume bad token to avoid infinite loop */
}

static void parse_postfix(Parser *p) {
    parse_primary(p);
    if (match(p, TOK_INC)) return;
    if (match(p, TOK_DEC)) return;
}

static void parse_unary(Parser *p) {
    if (match(p, TOK_NOT) || match(p, TOK_MINUS)) {
        parse_unary(p);
        return;
    }
    parse_postfix(p);
}

static void parse_multiplicative(Parser *p) {
    parse_unary(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH) || check(p, TOK_PERCENT)) {
        advance_p(p);
        parse_unary(p);
    }
}

static void parse_additive(Parser *p) {
    parse_multiplicative(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        advance_p(p);
        parse_multiplicative(p);
    }
}

static void parse_relational(Parser *p) {
    parse_additive(p);
    while (check(p, TOK_LT) || check(p, TOK_GT) ||
           check(p, TOK_LEQ) || check(p, TOK_GEQ)) {
        advance_p(p);
        parse_additive(p);
    }
}

static void parse_equality(Parser *p) {
    parse_relational(p);
    while (check(p, TOK_EQ) || check(p, TOK_NEQ)) {
        advance_p(p);
        parse_relational(p);
    }
}

static void parse_and_expr(Parser *p) {
    parse_equality(p);
    while (match(p, TOK_AND))
        parse_equality(p);
}

static void parse_or_expr(Parser *p) {
    parse_and_expr(p);
    while (match(p, TOK_OR))
        parse_and_expr(p);
}

static void parse_expression(Parser *p) {
    parse_or_expr(p);
    if (check(p, TOK_ASSIGN) || check(p, TOK_PLUS_ASSIGN) ||
        check(p, TOK_MINUS_ASSIGN)) {
        advance_p(p);
        parse_or_expr(p);
    }
}

/* ─── Statement parsing ───────────────────────────────────────────────────── */
static void parse_var_decl(Parser *p) {
    /* type already consumed by caller; consume IDENT */
    consume(p, TOK_IDENT, "Expected variable name after type");
    if (match(p, TOK_ASSIGN))
        parse_expression(p);
    consume(p, TOK_SEMICOLON, "Expected ';' after variable declaration");
}

static void parse_if_stmt(Parser *p) {
    consume(p, TOK_LPAREN, "Expected '(' after 'if'");
    parse_expression(p);
    consume(p, TOK_RPAREN, "Expected ')' after if condition");
    parse_block(p);
    if (match(p, TOK_ELSE))
        parse_block(p);
}

static void parse_while_stmt(Parser *p) {
    consume(p, TOK_LPAREN, "Expected '(' after 'while'");
    parse_expression(p);
    consume(p, TOK_RPAREN, "Expected ')' after while condition");
    parse_block(p);
}

static void parse_for_stmt(Parser *p) {
    consume(p, TOK_LPAREN, "Expected '(' after 'for'");

    /* init: optional var_decl or expr */
    if (is_type_token(cur(p)->type)) {
        advance_p(p); /* consume type */
        consume(p, TOK_IDENT, "Expected variable name in for-init");
        if (match(p, TOK_ASSIGN))
            parse_expression(p);
        consume(p, TOK_SEMICOLON, "Expected ';' after for-init");
    } else if (!check(p, TOK_SEMICOLON)) {
        parse_expression(p);
        consume(p, TOK_SEMICOLON, "Expected ';' after for-init");
    } else {
        advance_p(p); /* empty init */
    }

    /* condition */
    if (!check(p, TOK_SEMICOLON))
        parse_expression(p);
    consume(p, TOK_SEMICOLON, "Expected ';' after for-condition");

    /* update */
    if (!check(p, TOK_RPAREN))
        parse_expression(p);
    consume(p, TOK_RPAREN, "Expected ')' after for-update");

    parse_block(p);
}

static void parse_return_stmt(Parser *p) {
    if (!check(p, TOK_SEMICOLON))
        parse_expression(p);
    consume(p, TOK_SEMICOLON, "Expected ';' after return value");
}

static void parse_input_stmt(Parser *p) {
    consume(p, TOK_IDENT, "Expected identifier after 'input'");
    consume(p, TOK_SEMICOLON, "Expected ';' after input statement");
}

static void parse_output_stmt(Parser *p) {
    parse_expression(p);
    consume(p, TOK_SEMICOLON, "Expected ';' after output statement");
}

static void parse_statement(Parser *p) {
    if (is_type_token(cur(p)->type)) {
        TokenType t = cur(p)->type;
        advance_p(p);
        
        (void)t;
        parse_var_decl(p);
        return;
    }

    switch (cur(p)->type) {
        case TOK_IF:     advance_p(p); parse_if_stmt(p);     break;
        case TOK_WHILE:  advance_p(p); parse_while_stmt(p);  break;
        case TOK_FOR:    advance_p(p); parse_for_stmt(p);    break;
        case TOK_RETURN: advance_p(p); parse_return_stmt(p); break;
        case TOK_INPUT:  advance_p(p); parse_input_stmt(p);  break;
        case TOK_OUTPUT: advance_p(p); parse_output_stmt(p); break;
        case TOK_RBRACE:  break;
        case TOK_EOF:    break;
        default:
            parse_expression(p);
            consume(p, TOK_SEMICOLON, "Expected ';' after expression");
            break;
    }
}

static void parse_block(Parser *p) {
    consume(p, TOK_LBRACE, "Expected '{' to begin block");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF))
        parse_statement(p);
    consume(p, TOK_RBRACE, "Expected '}' to close block");
}

/* ─── Function declaration ────────────────────────────────────────────────── */
static void parse_param_list(Parser *p) {
    if (check(p, TOK_RPAREN)) return;
    /* first param */
    if (!is_type_token(cur(p)->type)) {
        err_report(p->err, ERR_SYNTAX, cur(p)->line,
                   "Expected type in parameter list, got '%s'", cur(p)->lexeme);
        return;
    }
    advance_p(p);
    consume(p, TOK_IDENT, "Expected parameter name");

    while (match(p, TOK_COMMA)) {
        if (!is_type_token(cur(p)->type)) {
            err_report(p->err, ERR_SYNTAX, cur(p)->line,
                       "Expected type in parameter list, got '%s'", cur(p)->lexeme);
            return;
        }
        advance_p(p);
        consume(p, TOK_IDENT, "Expected parameter name");
    }
}

static void parse_func_decl(Parser *p) {
    /* type already consumed */
    consume(p, TOK_IDENT, "Expected function name");
    consume(p, TOK_LPAREN, "Expected '(' after function name");
    parse_param_list(p);
    consume(p, TOK_RPAREN, "Expected ')' after parameter list");
    parse_block(p);
}

/* ─── Top-level declaration ───────────────────────────────────────────────── */
static void parse_declaration(Parser *p) {
    if (!is_type_token(cur(p)->type)) {
        err_report(p->err, ERR_SYNTAX, cur(p)->line,
                   "Expected type at top-level declaration, got '%s'",
                   cur(p)->lexeme);
        synchronize(p);
        return;
    }
    advance_p(p); /* consume type */

    /* peek: type IDENT '(' → function; type IDENT → var */
    if (check(p, TOK_IDENT) && p->pos + 1 < p->count &&
        p->tokens[p->pos + 1].type == TOK_LPAREN) {
        parse_func_decl(p);
    } else {
        parse_var_decl(p);
    }
}

/* ─── Public API ──────────────────────────────────────────────────────────── */
void parser_init(Parser *p, Lexer *lex, ErrorHandler *err) {
    p->tokens = lex->tokens;
    p->count  = lex->token_count;
    p->pos    = 0;
    p->err    = err;
}

void parser_parse(Parser *p) {
    printf("\n[Parser] Starting syntax analysis...\n");
    while (!check(p, TOK_EOF))
        parse_declaration(p);
    if (err_count(p->err) == 0)
        printf("[Parser] Syntax analysis completed successfully.\n");
    else
        printf("[Parser] Syntax analysis completed with errors.\n");
}
