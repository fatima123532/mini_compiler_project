#include "semantic.h"

/* ─── Helpers ─────────────────────────────────────────────────────────────── */
static Token *scur(Semantic *s)  { return &s->tokens[s->pos]; }
static Token *sprev(Semantic *s) { return &s->tokens[s->pos - 1]; }

static Token *sadvance(Semantic *s) {
    if (s->pos < s->count - 1) s->pos++;
    return sprev(s);
}

static int scheck(Semantic *s, TokenType t) { return scur(s)->type == t; }

static int smatch(Semantic *s, TokenType t) {
    if (scheck(s, t)) { sadvance(s); return 1; }
    return 0;
}

static int is_type_tok(TokenType t) {
    return t == TOK_INT || t == TOK_FLOAT || t == TOK_BOOL || t == TOK_VOID;
}

static DataType tok_to_dt(TokenType t) {
    switch (t) {
        case TOK_INT:   return DT_INT;
        case TOK_FLOAT: return DT_FLOAT;
        case TOK_BOOL:  return DT_BOOL;
        case TOK_VOID:  return DT_VOID;
        default:        return DT_UNKNOWN;
    }
}

/* ─── Forward declarations ────────────────────────────────────────────────── */
static DataType sem_expression(Semantic *s);
static void     sem_statement (Semantic *s);
static void     sem_block     (Semantic *s);

/* ─── Expression type inference ───────────────────────────────────────────── */
static DataType sem_primary(Semantic *s) {
    if (smatch(s, TOK_INT_LIT))   return DT_INT;
    if (smatch(s, TOK_FLOAT_LIT)) return DT_FLOAT;
    if (smatch(s, TOK_TRUE) || smatch(s, TOK_FALSE)) return DT_BOOL;

    if (scheck(s, TOK_IDENT)) {
        char name[256];
        strncpy(name, scur(s)->lexeme, 255);
        name[255] = '\0';
        int line = scur(s)->line;
        sadvance(s);

        /* function call */
        if (smatch(s, TOK_LPAREN)) {
            Symbol *fsym = sym_lookup(s->sym, name);
            if (!fsym) {
                err_report(s->err, ERR_SEMANTIC, line,
                           "Call to undeclared function '%s'", name);
                /* consume args anyway */
                int depth = 1;
                while (!scheck(s, TOK_EOF) && depth > 0) {
                    if (scheck(s, TOK_LPAREN)) depth++;
                    if (scheck(s, TOK_RPAREN)) { depth--; if (depth == 0) break; }
                    sadvance(s);
                }
                smatch(s, TOK_RPAREN);
                return DT_UNKNOWN;
            }
            if (fsym->kind != SYM_FUNC) {
                err_report(s->err, ERR_SEMANTIC, line,
                           "'%s' is not a function", name);
            }
            /* parse arguments and check arity */
            int arg_count = 0;
            if (!scheck(s, TOK_RPAREN)) {
                sem_expression(s); arg_count++;
                while (smatch(s, TOK_COMMA)) {
                    sem_expression(s); arg_count++;
                }
            }
            smatch(s, TOK_RPAREN);
            if (fsym && fsym->kind == SYM_FUNC &&
                arg_count != fsym->param_count) {
                err_report(s->err, ERR_SEMANTIC, line,
                           "Function '%s' expects %d argument(s), got %d",
                           name, fsym->param_count, arg_count);
            }
            return fsym ? fsym->dtype : DT_UNKNOWN;
        }

        /* variable reference */
        Symbol *vsym = sym_lookup(s->sym, name);
        if (!vsym) {
            err_report(s->err, ERR_SEMANTIC, line,
                       "Use of undeclared variable '%s'", name);
            return DT_UNKNOWN;
        }
        vsym->is_initialized = 1; /* mark as used */
        return vsym->dtype;
    }

    if (smatch(s, TOK_LPAREN)) {
        DataType dt = sem_expression(s);
        smatch(s, TOK_RPAREN);
        return dt;
    }

    sadvance(s); /* consume unknown */
    return DT_UNKNOWN;
}

static DataType sem_postfix(Semantic *s) {
    DataType dt = sem_primary(s);
    if (smatch(s, TOK_INC) || smatch(s, TOK_DEC)) {
        if (dt != DT_INT && dt != DT_FLOAT)
            err_report(s->err, ERR_SEMANTIC, sprev(s)->line,
                       "Increment/decrement requires numeric type");
    }
    return dt;
}

static DataType sem_unary(Semantic *s) {
    if (smatch(s, TOK_NOT)) {
        DataType dt = sem_unary(s);
        if (dt != DT_BOOL && dt != DT_UNKNOWN)
            err_report(s->err, ERR_SEMANTIC, sprev(s)->line,
                       "Operator '!' requires bool operand");
        return DT_BOOL;
    }
    if (smatch(s, TOK_MINUS)) {
        DataType dt = sem_unary(s);
        if (dt != DT_INT && dt != DT_FLOAT && dt != DT_UNKNOWN)
            err_report(s->err, ERR_SEMANTIC, sprev(s)->line,
                       "Unary '-' requires numeric operand");
        return dt;
    }
    return sem_postfix(s);
}

static DataType sem_multiplicative(Semantic *s) {
    DataType left = sem_unary(s);
    while (scheck(s, TOK_STAR) || scheck(s, TOK_SLASH) || scheck(s, TOK_PERCENT)) {
        int line = scur(s)->line;
        sadvance(s);
        DataType right = sem_unary(s);
        if ((left  != DT_INT && left  != DT_FLOAT && left  != DT_UNKNOWN) ||
            (right != DT_INT && right != DT_FLOAT && right != DT_UNKNOWN))
            err_report(s->err, ERR_SEMANTIC, line,
                       "Arithmetic operator requires numeric operands");
        if (left == DT_FLOAT || right == DT_FLOAT) left = DT_FLOAT;
    }
    return left;
}

static DataType sem_additive(Semantic *s) {
    DataType left = sem_multiplicative(s);
    while (scheck(s, TOK_PLUS) || scheck(s, TOK_MINUS)) {
        int line = scur(s)->line;
        sadvance(s);
        DataType right = sem_multiplicative(s);
        if ((left  != DT_INT && left  != DT_FLOAT && left  != DT_UNKNOWN) ||
            (right != DT_INT && right != DT_FLOAT && right != DT_UNKNOWN))
            err_report(s->err, ERR_SEMANTIC, line,
                       "Arithmetic operator requires numeric operands");
        if (left == DT_FLOAT || right == DT_FLOAT) left = DT_FLOAT;
    }
    return left;
}

static DataType sem_relational(Semantic *s) {
    DataType left = sem_additive(s);
    while (scheck(s, TOK_LT) || scheck(s, TOK_GT) ||
           scheck(s, TOK_LEQ) || scheck(s, TOK_GEQ)) {
        int line = scur(s)->line;
        sadvance(s);
        DataType right = sem_additive(s);
        if ((left  != DT_INT && left  != DT_FLOAT && left  != DT_UNKNOWN) ||
            (right != DT_INT && right != DT_FLOAT && right != DT_UNKNOWN))
            err_report(s->err, ERR_SEMANTIC, line,
                       "Relational operator requires numeric operands");
        left = DT_BOOL;
    }
    return left;
}

static DataType sem_equality(Semantic *s) {
    DataType left = sem_relational(s);
    while (scheck(s, TOK_EQ) || scheck(s, TOK_NEQ)) {
        int line = scur(s)->line;
        sadvance(s);
        DataType right = sem_relational(s);
        if (left != DT_UNKNOWN && right != DT_UNKNOWN && left != right)
            err_report(s->err, ERR_SEMANTIC, line,
                       "Type mismatch in equality: '%s' vs '%s'",
                       datatype_str(left), datatype_str(right));
        left = DT_BOOL;
    }
    return left;
}

static DataType sem_and_expr(Semantic *s) {
    DataType left = sem_equality(s);
    while (smatch(s, TOK_AND)) {
        int line = sprev(s)->line;
        DataType right = sem_equality(s);
        if ((left  != DT_BOOL && left  != DT_UNKNOWN) ||
            (right != DT_BOOL && right != DT_UNKNOWN))
            err_report(s->err, ERR_SEMANTIC, line,
                       "Operator '&&' requires bool operands");
        left = DT_BOOL;
    }
    return left;
}

static DataType sem_or_expr(Semantic *s) {
    DataType left = sem_and_expr(s);
    while (smatch(s, TOK_OR)) {
        int line = sprev(s)->line;
        DataType right = sem_and_expr(s);
        if ((left  != DT_BOOL && left  != DT_UNKNOWN) ||
            (right != DT_BOOL && right != DT_UNKNOWN))
            err_report(s->err, ERR_SEMANTIC, line,
                       "Operator '||' requires bool operands");
        left = DT_BOOL;
    }
    return left;
}

static DataType sem_expression(Semantic *s) {
    DataType left = sem_or_expr(s);

    if (scheck(s, TOK_ASSIGN) || scheck(s, TOK_PLUS_ASSIGN) ||
        scheck(s, TOK_MINUS_ASSIGN)) {
        int line = scur(s)->line;
        sadvance(s);
        DataType right = sem_or_expr(s);
        /* Type compatibility: int ↔ float allowed (widening), bool strict */
        if (left != DT_UNKNOWN && right != DT_UNKNOWN) {
            int ok = (left == right) ||
                     (left == DT_FLOAT && right == DT_INT) ||
                     (left == DT_INT   && right == DT_FLOAT);
            if (!ok)
                err_report(s->err, ERR_SEMANTIC, line,
                           "Type mismatch in assignment: cannot assign '%s' to '%s'",
                           datatype_str(right), datatype_str(left));
        }
        return left;
    }
    return left;
}

/* ─── Statement analysis ──────────────────────────────────────────────────── */
static void sem_var_decl(Semantic *s, DataType dt) {
    if (!scheck(s, TOK_IDENT)) { sadvance(s); return; }
    char name[256];
    strncpy(name, scur(s)->lexeme, 255); name[255] = '\0';
    int line = scur(s)->line;
    sadvance(s); /* consume IDENT */

    if (!sym_insert(s->sym, name, dt, SYM_VAR, line))
        err_report(s->err, ERR_SEMANTIC, line,
                   "Redeclaration of '%s' in the same scope", name);

    if (smatch(s, TOK_ASSIGN)) {
        DataType rhs = sem_expression(s);
        Symbol *sym = sym_lookup(s->sym, name);
        if (sym) sym->is_initialized = 1;
        if (dt != DT_UNKNOWN && rhs != DT_UNKNOWN && dt != rhs) {
            int ok = (dt == DT_FLOAT && rhs == DT_INT) ||
                     (dt == DT_INT   && rhs == DT_FLOAT);
            if (!ok)
                err_report(s->err, ERR_SEMANTIC, line,
                           "Type mismatch: cannot initialize '%s' (%s) with %s",
                           name, datatype_str(dt), datatype_str(rhs));
        }
    }
    smatch(s, TOK_SEMICOLON);
}

static void sem_if_stmt(Semantic *s) {
    smatch(s, TOK_LPAREN);
    DataType cond = sem_expression(s);
    smatch(s, TOK_RPAREN);
    if (cond != DT_BOOL && cond != DT_UNKNOWN)
        err_report(s->err, ERR_SEMANTIC, sprev(s)->line,
                   "If condition must be bool, got '%s'", datatype_str(cond));
    sem_block(s);
    if (smatch(s, TOK_ELSE)) sem_block(s);
}

static void sem_while_stmt(Semantic *s) {
    smatch(s, TOK_LPAREN);
    DataType cond = sem_expression(s);
    smatch(s, TOK_RPAREN);
    if (cond != DT_BOOL && cond != DT_UNKNOWN)
        err_report(s->err, ERR_SEMANTIC, sprev(s)->line,
                   "While condition must be bool, got '%s'", datatype_str(cond));
    sem_block(s);
}

static void sem_for_stmt(Semantic *s) {
    smatch(s, TOK_LPAREN);
    sym_enter_scope(s->sym);

    /* init */
    if (is_type_tok(scur(s)->type)) {
        DataType dt = tok_to_dt(scur(s)->type);
        sadvance(s);
        sem_var_decl(s, dt);
    } else if (!scheck(s, TOK_SEMICOLON)) {
        sem_expression(s);
        smatch(s, TOK_SEMICOLON);
    } else {
        smatch(s, TOK_SEMICOLON);
    }

    /* condition */
    if (!scheck(s, TOK_SEMICOLON)) {
        DataType cond = sem_expression(s);
        if (cond != DT_BOOL && cond != DT_UNKNOWN)
            err_report(s->err, ERR_SEMANTIC, scur(s)->line,
                       "For condition must be bool, got '%s'", datatype_str(cond));
    }
    smatch(s, TOK_SEMICOLON);

    /* update */
    if (!scheck(s, TOK_RPAREN))
        sem_expression(s);
    smatch(s, TOK_RPAREN);

    /* body (re-use the scope we already opened) */
    smatch(s, TOK_LBRACE);
    while (!scheck(s, TOK_RBRACE) && !scheck(s, TOK_EOF))
        sem_statement(s);
    smatch(s, TOK_RBRACE);

    sym_exit_scope(s->sym);
}

static void sem_return_stmt(Semantic *s) {
    int line = sprev(s)->line;
    if (!scheck(s, TOK_SEMICOLON)) {
        DataType rt = sem_expression(s);
        if (s->cur_func_return_type != DT_UNKNOWN &&
            rt != DT_UNKNOWN &&
            rt != s->cur_func_return_type) {
            int ok = (s->cur_func_return_type == DT_FLOAT && rt == DT_INT);
            if (!ok)
                err_report(s->err, ERR_SEMANTIC, line,
                           "Return type mismatch in '%s': expected '%s', got '%s'",
                           s->cur_func_name,
                           datatype_str(s->cur_func_return_type),
                           datatype_str(rt));
        }
    } else if (s->cur_func_return_type != DT_VOID &&
               s->cur_func_return_type != DT_UNKNOWN) {
        err_report(s->err, ERR_SEMANTIC, line,
                   "Function '%s' must return a value of type '%s'",
                   s->cur_func_name,
                   datatype_str(s->cur_func_return_type));
    }
    smatch(s, TOK_SEMICOLON);
}

static void sem_input_stmt(Semantic *s) {
    int line = scur(s)->line;
    if (scheck(s, TOK_IDENT)) {
        Symbol *sym = sym_lookup(s->sym, scur(s)->lexeme);
        if (!sym)
            err_report(s->err, ERR_SEMANTIC, line,
                       "Use of undeclared variable '%s' in input",
                       scur(s)->lexeme);
        else
            sym->is_initialized = 1;
        sadvance(s);
    }
    smatch(s, TOK_SEMICOLON);
}

static void sem_output_stmt(Semantic *s) {
    sem_expression(s);
    smatch(s, TOK_SEMICOLON);
}

static void sem_statement(Semantic *s) {
    if (is_type_tok(scur(s)->type)) {
        DataType dt = tok_to_dt(scur(s)->type);
        sadvance(s);
        sem_var_decl(s, dt);
        return;
    }
    switch (scur(s)->type) {
        case TOK_IF:     sadvance(s); sem_if_stmt(s);     break;
        case TOK_WHILE:  sadvance(s); sem_while_stmt(s);  break;
        case TOK_FOR:    sadvance(s); sem_for_stmt(s);    break;
        case TOK_RETURN: sadvance(s); sem_return_stmt(s); break;
        case TOK_INPUT:  sadvance(s); sem_input_stmt(s);  break;
        case TOK_OUTPUT: sadvance(s); sem_output_stmt(s); break;
        case TOK_RBRACE: break;
        case TOK_EOF:    break;
        default:
            sem_expression(s);
            smatch(s, TOK_SEMICOLON);
            break;
    }
}

static void sem_block(Semantic *s) {
    smatch(s, TOK_LBRACE);
    sym_enter_scope(s->sym);
    while (!scheck(s, TOK_RBRACE) && !scheck(s, TOK_EOF))
        sem_statement(s);
    smatch(s, TOK_RBRACE);
    sym_exit_scope(s->sym);
}

/* ─── Function declaration ────────────────────────────────────────────────── */
static void sem_func_decl(Semantic *s, DataType ret_type) {
    char name[256];
    strncpy(name, scur(s)->lexeme, 255); name[255] = '\0';
    int line = scur(s)->line;
    sadvance(s); /* IDENT */

    /* Register function in symbol table BEFORE entering its scope */
    if (!sym_insert(s->sym, name, ret_type, SYM_FUNC, line))
        err_report(s->err, ERR_SEMANTIC, line,
                   "Redeclaration of function '%s'", name);

    Symbol *fsym = sym_lookup(s->sym, name);
    s->cur_func_return_type = ret_type;
    strncpy(s->cur_func_name, name, 255);

    smatch(s, TOK_LPAREN);
    sym_enter_scope(s->sym);

    /* Parameters */
    int param_count = 0;
    while (!scheck(s, TOK_RPAREN) && !scheck(s, TOK_EOF)) {
        if (!is_type_tok(scur(s)->type)) { sadvance(s); break; }
        DataType pdt = tok_to_dt(scur(s)->type);
        sadvance(s);

        if (scheck(s, TOK_IDENT)) {
            char pname[256];
            strncpy(pname, scur(s)->lexeme, 255); pname[255] = '\0';
            int pline = scur(s)->line;
            sadvance(s);
            sym_insert(s->sym, pname, pdt, SYM_PARAM, pline);
            Symbol *psym = sym_lookup(s->sym, pname);
            if (psym) psym->is_initialized = 1;
            if (fsym && param_count < MAX_PARAMS)
                fsym->param_types[param_count] = pdt;
            param_count++;
        }
        smatch(s, TOK_COMMA);
    }
    smatch(s, TOK_RPAREN);
    if (fsym) fsym->param_count = param_count;

    /* Body — reuse the param scope */
    smatch(s, TOK_LBRACE);
    while (!scheck(s, TOK_RBRACE) && !scheck(s, TOK_EOF))
        sem_statement(s);
    smatch(s, TOK_RBRACE);
    sym_exit_scope(s->sym);
}

/* ─── Top-level ───────────────────────────────────────────────────────────── */
static void sem_declaration(Semantic *s) {
    if (!is_type_tok(scur(s)->type)) { sadvance(s); return; }
    DataType dt = tok_to_dt(scur(s)->type);
    sadvance(s);

    /* function or variable? */
    if (scheck(s, TOK_IDENT) && s->pos + 1 < s->count &&
        s->tokens[s->pos + 1].type == TOK_LPAREN) {
        sem_func_decl(s, dt);
    } else {
        sem_var_decl(s, dt);
    }
}

/* ─── Public API ──────────────────────────────────────────────────────────── */
void semantic_init(Semantic *s, Lexer *lex, SymbolTable *sym,
                   ErrorHandler *err) {
    s->tokens               = lex->tokens;
    s->count                = lex->token_count;
    s->pos                  = 0;
    s->sym                  = sym;
    s->err                  = err;
    s->cur_func_return_type = DT_UNKNOWN;
    s->cur_func_name[0]     = '\0';
}

void semantic_analyze(Semantic *s) {
    printf("[Semantic] Starting semantic analysis...\n");
    while (!scheck(s, TOK_EOF))
        sem_declaration(s);
    if (err_count(s->err) == 0)
        printf("[Semantic] Semantic analysis completed successfully.\n");
    else
        printf("[Semantic] Semantic analysis completed with errors.\n");
}
