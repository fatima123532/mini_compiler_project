#include "codegen.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ── Helpers ──────────────────────────────────────────────────────────────── */
static Token *cgcur (CodeGen *cg) { return &cg->tokens[cg->pos]; }
static Token *cgprev(CodeGen *cg) { return &cg->tokens[cg->pos - 1]; }

static Token *cgadvance(CodeGen *cg) {
    if (cg->pos < cg->count - 1) cg->pos++;
    return cgprev(cg);
}

static int cgcheck(CodeGen *cg, TokenType t) { return cgcur(cg)->type == t; }

static int cgmatch(CodeGen *cg, TokenType t) {
    if (cgcheck(cg, t)) { cgadvance(cg); return 1; }
    return 0;
}

static int is_type_tok(TokenType t) {
    return t == TOK_INT || t == TOK_FLOAT || t == TOK_BOOL || t == TOK_VOID;
}

/* ── New temp / label ─────────────────────────────────────────────────────── */
static void new_temp(CodeGen *cg, char *out) {
    sprintf(out, "t%d", cg->temp_count++);
}

static void new_label(CodeGen *cg, char *out) {
    sprintf(out, "L%d", cg->label_count++);
}

/* ── Emit a TAC instruction ───────────────────────────────────────────────── */
static void emit(CodeGen *cg, TACOp op,
                 const char *result, const char *arg1, const char *arg2) {
    if (cg->code_count >= MAX_TAC) return;
    TACInstr *i = &cg->code[cg->code_count++];
    i->op = op;
    strncpy(i->result, result ? result : "", MAX_TAC_NAME - 1);
    strncpy(i->arg1,   arg1   ? arg1   : "", MAX_TAC_NAME - 1);
    strncpy(i->arg2,   arg2   ? arg2   : "", MAX_TAC_NAME - 1);
    i->result[MAX_TAC_NAME-1] = '\0';
    i->arg1  [MAX_TAC_NAME-1] = '\0';
    i->arg2  [MAX_TAC_NAME-1] = '\0';
}

/* ── Forward declarations ─────────────────────────────────────────────────── */
static void cg_expr     (CodeGen *cg, char *out);
static void cg_statement(CodeGen *cg);
static void cg_block    (CodeGen *cg);

/* ── Expression code generation ───────────────────────────────────────────── */

static void cg_primary(CodeGen *cg, char *out) {
    /* Integer literal */
    if (cgcheck(cg, TOK_INT_LIT)) {
        strncpy(out, cgcur(cg)->lexeme, MAX_TAC_NAME - 1);
        cgadvance(cg);
        return;
    }
    /* Float literal */
    if (cgcheck(cg, TOK_FLOAT_LIT)) {
        strncpy(out, cgcur(cg)->lexeme, MAX_TAC_NAME - 1);
        cgadvance(cg);
        return;
    }
    /* Bool literals */
    if (cgcheck(cg, TOK_TRUE))  { strncpy(out, "1", MAX_TAC_NAME-1); cgadvance(cg); return; }
    if (cgcheck(cg, TOK_FALSE)) { strncpy(out, "0", MAX_TAC_NAME-1); cgadvance(cg); return; }

    /* Identifier or function call */
    if (cgcheck(cg, TOK_IDENT)) {
        char name[MAX_TAC_NAME];
        strncpy(name, cgcur(cg)->lexeme, MAX_TAC_NAME - 1);
        name[MAX_TAC_NAME-1] = '\0';
        cgadvance(cg);

        if (cgmatch(cg, TOK_LPAREN)) {
            /* Function call: collect args, then emit CALL */
            int argc = 0;
            char arg_tmp[MAX_TAC_NAME];

            if (!cgcheck(cg, TOK_RPAREN)) {
                cg_expr(cg, arg_tmp);
                emit(cg, TAC_PARAM, "", arg_tmp, "");
                argc++;
                while (cgmatch(cg, TOK_COMMA)) {
                    cg_expr(cg, arg_tmp);
                    emit(cg, TAC_PARAM, "", arg_tmp, "");
                    argc++;
                }
            }
            cgmatch(cg, TOK_RPAREN);

            char count_str[16];
            sprintf(count_str, "%d", argc);
            new_temp(cg, out);
            emit(cg, TAC_CALL, out, name, count_str);
            return;
        }

        /* Plain variable reference */
        strncpy(out, name, MAX_TAC_NAME - 1);
        return;
    }

    /* Parenthesised expression */
    if (cgmatch(cg, TOK_LPAREN)) {
        cg_expr(cg, out);
        cgmatch(cg, TOK_RPAREN);
        return;
    }

    /* Fallback */
    strncpy(out, "?", MAX_TAC_NAME - 1);
    cgadvance(cg);
}

static void cg_postfix(CodeGen *cg, char *out) {
    cg_primary(cg, out);

    if (cgmatch(cg, TOK_INC)) {
        char tmp[MAX_TAC_NAME];
        new_temp(cg, tmp);
        emit(cg, TAC_ASSIGN, tmp, out, "");   /* save original */
        emit(cg, TAC_INC, out, out, "");       /* out = out + 1  */
        strncpy(out, tmp, MAX_TAC_NAME - 1);
        return;
    }
    if (cgmatch(cg, TOK_DEC)) {
        char tmp[MAX_TAC_NAME];
        new_temp(cg, tmp);
        emit(cg, TAC_ASSIGN, tmp, out, "");
        emit(cg, TAC_DEC, out, out, "");
        strncpy(out, tmp, MAX_TAC_NAME - 1);
        return;
    }
}

static void cg_unary(CodeGen *cg, char *out) {
    if (cgmatch(cg, TOK_NOT)) {
        char src[MAX_TAC_NAME];
        cg_unary(cg, src);
        new_temp(cg, out);
        emit(cg, TAC_NOT, out, src, "");
        return;
    }
    if (cgmatch(cg, TOK_MINUS)) {
        char src[MAX_TAC_NAME];
        cg_unary(cg, src);
        new_temp(cg, out);
        emit(cg, TAC_NEG, out, src, "");
        return;
    }
    cg_postfix(cg, out);
}

static void cg_multiplicative(CodeGen *cg, char *out) {
    cg_unary(cg, out);
    while (cgcheck(cg, TOK_STAR) || cgcheck(cg, TOK_SLASH) || cgcheck(cg, TOK_PERCENT)) {
        TACOp op = cgcheck(cg, TOK_STAR)    ? TAC_MUL :
                   cgcheck(cg, TOK_SLASH)   ? TAC_DIV : TAC_MOD;
        cgadvance(cg);
        char right[MAX_TAC_NAME], tmp[MAX_TAC_NAME];
        cg_unary(cg, right);
        new_temp(cg, tmp);
        emit(cg, op, tmp, out, right);
        strncpy(out, tmp, MAX_TAC_NAME - 1);
    }
}

static void cg_additive(CodeGen *cg, char *out) {
    cg_multiplicative(cg, out);
    while (cgcheck(cg, TOK_PLUS) || cgcheck(cg, TOK_MINUS)) {
        TACOp op = cgcheck(cg, TOK_PLUS) ? TAC_ADD : TAC_SUB;
        cgadvance(cg);
        char right[MAX_TAC_NAME], tmp[MAX_TAC_NAME];
        cg_multiplicative(cg, right);
        new_temp(cg, tmp);
        emit(cg, op, tmp, out, right);
        strncpy(out, tmp, MAX_TAC_NAME - 1);
    }
}

static void cg_relational(CodeGen *cg, char *out) {
    cg_additive(cg, out);
    while (cgcheck(cg, TOK_LT)  || cgcheck(cg, TOK_GT) ||
           cgcheck(cg, TOK_LEQ) || cgcheck(cg, TOK_GEQ)) {
        TACOp op = cgcheck(cg, TOK_LT)  ? TAC_LT  :
                   cgcheck(cg, TOK_GT)  ? TAC_GT  :
                   cgcheck(cg, TOK_LEQ) ? TAC_LEQ : TAC_GEQ;
        cgadvance(cg);
        char right[MAX_TAC_NAME], tmp[MAX_TAC_NAME];
        cg_additive(cg, right);
        new_temp(cg, tmp);
        emit(cg, op, tmp, out, right);
        strncpy(out, tmp, MAX_TAC_NAME - 1);
    }
}

static void cg_equality(CodeGen *cg, char *out) {
    cg_relational(cg, out);
    while (cgcheck(cg, TOK_EQ) || cgcheck(cg, TOK_NEQ)) {
        TACOp op = cgcheck(cg, TOK_EQ) ? TAC_EQ : TAC_NEQ;
        cgadvance(cg);
        char right[MAX_TAC_NAME], tmp[MAX_TAC_NAME];
        cg_relational(cg, right);
        new_temp(cg, tmp);
        emit(cg, op, tmp, out, right);
        strncpy(out, tmp, MAX_TAC_NAME - 1);
    }
}

static void cg_and_expr(CodeGen *cg, char *out) {
    cg_equality(cg, out);
    while (cgmatch(cg, TOK_AND)) {
        char right[MAX_TAC_NAME], tmp[MAX_TAC_NAME];
        cg_equality(cg, right);
        new_temp(cg, tmp);
        emit(cg, TAC_AND, tmp, out, right);
        strncpy(out, tmp, MAX_TAC_NAME - 1);
    }
}

static void cg_or_expr(CodeGen *cg, char *out) {
    cg_and_expr(cg, out);
    while (cgmatch(cg, TOK_OR)) {
        char right[MAX_TAC_NAME], tmp[MAX_TAC_NAME];
        cg_and_expr(cg, right);
        new_temp(cg, tmp);
        emit(cg, TAC_OR, tmp, out, right);
        strncpy(out, tmp, MAX_TAC_NAME - 1);
    }
}

static void cg_expr(CodeGen *cg, char *out) {
    cg_or_expr(cg, out);

    if (cgcheck(cg, TOK_ASSIGN)) {
        cgadvance(cg);
        char rhs[MAX_TAC_NAME];
        cg_or_expr(cg, rhs);
        emit(cg, TAC_ASSIGN, out, rhs, "");
        return;
    }
    if (cgcheck(cg, TOK_PLUS_ASSIGN)) {
        cgadvance(cg);
        char rhs[MAX_TAC_NAME], tmp[MAX_TAC_NAME];
        cg_or_expr(cg, rhs);
        new_temp(cg, tmp);
        emit(cg, TAC_ADD, tmp, out, rhs);
        emit(cg, TAC_ASSIGN, out, tmp, "");
        return;
    }
    if (cgcheck(cg, TOK_MINUS_ASSIGN)) {
        cgadvance(cg);
        char rhs[MAX_TAC_NAME], tmp[MAX_TAC_NAME];
        cg_or_expr(cg, rhs);
        new_temp(cg, tmp);
        emit(cg, TAC_SUB, tmp, out, rhs);
        emit(cg, TAC_ASSIGN, out, tmp, "");
        return;
    }
}

/* ── Statement code generation ────────────────────────────────────────────── */

static void cg_var_decl(CodeGen *cg) {
    /* type already consumed by caller */
    if (!cgcheck(cg, TOK_IDENT)) { cgadvance(cg); return; }
    char name[MAX_TAC_NAME];
    strncpy(name, cgcur(cg)->lexeme, MAX_TAC_NAME - 1);
    name[MAX_TAC_NAME-1] = '\0';
    cgadvance(cg);

    if (cgmatch(cg, TOK_ASSIGN)) {
        char rhs[MAX_TAC_NAME];
        cg_expr(cg, rhs);
        emit(cg, TAC_ASSIGN, name, rhs, "");
    }
    cgmatch(cg, TOK_SEMICOLON);
}

static void cg_if_stmt(CodeGen *cg) {
    char cond[MAX_TAC_NAME], else_lbl[MAX_TAC_NAME], end_lbl[MAX_TAC_NAME];
    new_label(cg, else_lbl);
    new_label(cg, end_lbl);

    cgmatch(cg, TOK_LPAREN);
    cg_expr(cg, cond);
    cgmatch(cg, TOK_RPAREN);

    emit(cg, TAC_IF_FALSE, "", cond, else_lbl);
    cg_block(cg);

    if (cgcheck(cg, TOK_ELSE)) {
        emit(cg, TAC_GOTO, "", end_lbl, "");
        emit(cg, TAC_LABEL, else_lbl, "", "");
        cgadvance(cg);
        cg_block(cg);
        emit(cg, TAC_LABEL, end_lbl, "", "");
    } else {
        emit(cg, TAC_LABEL, else_lbl, "", "");
    }
}

static void cg_while_stmt(CodeGen *cg) {
    char start_lbl[MAX_TAC_NAME], end_lbl[MAX_TAC_NAME], cond[MAX_TAC_NAME];
    new_label(cg, start_lbl);
    new_label(cg, end_lbl);

    emit(cg, TAC_LABEL, start_lbl, "", "");

    cgmatch(cg, TOK_LPAREN);
    cg_expr(cg, cond);
    cgmatch(cg, TOK_RPAREN);

    emit(cg, TAC_IF_FALSE, "", cond, end_lbl);
    cg_block(cg);
    emit(cg, TAC_GOTO, "", start_lbl, "");
    emit(cg, TAC_LABEL, end_lbl, "", "");
}

static void cg_for_stmt(CodeGen *cg) {
    char start_lbl[MAX_TAC_NAME], end_lbl[MAX_TAC_NAME];
    char cond[MAX_TAC_NAME];
    new_label(cg, start_lbl);
    new_label(cg, end_lbl);

    cgmatch(cg, TOK_LPAREN);

    /* Init */
    if (is_type_tok(cgcur(cg)->type)) {
        cgadvance(cg);
        cg_var_decl(cg);
    } else if (!cgcheck(cg, TOK_SEMICOLON)) {
        char tmp[MAX_TAC_NAME];
        cg_expr(cg, tmp);
        cgmatch(cg, TOK_SEMICOLON);
    } else {
        cgmatch(cg, TOK_SEMICOLON);
    }

    /* Condition */
    emit(cg, TAC_LABEL, start_lbl, "", "");
    if (!cgcheck(cg, TOK_SEMICOLON)) {
        cg_expr(cg, cond);
        emit(cg, TAC_IF_FALSE, "", cond, end_lbl);
    }
    cgmatch(cg, TOK_SEMICOLON);

    /* Save position of update expression, skip it for now */
    int update_pos = cg->pos;
    /* Skip update expression tokens */
    int depth = 0;
    while (!cgcheck(cg, TOK_EOF)) {
        if (cgcheck(cg, TOK_LPAREN)) depth++;
        if (cgcheck(cg, TOK_RPAREN)) {
            if (depth == 0) break;
            depth--;
        }
        cgadvance(cg);
    }
    cgmatch(cg, TOK_RPAREN);

    /* Body */
    cg_block(cg);

    /* Emit update expression now */
    int after_body = cg->pos;
    cg->pos = update_pos;
    if (!cgcheck(cg, TOK_RPAREN)) {
        char tmp[MAX_TAC_NAME];
        cg_expr(cg, tmp);
    }
    cg->pos = after_body;

    emit(cg, TAC_GOTO, "", start_lbl, "");
    emit(cg, TAC_LABEL, end_lbl, "", "");
}

static void cg_return_stmt(CodeGen *cg) {
    if (!cgcheck(cg, TOK_SEMICOLON)) {
        char val[MAX_TAC_NAME];
        cg_expr(cg, val);
        emit(cg, TAC_RETURN, "", val, "");
    } else {
        emit(cg, TAC_RETURN, "", "", "");
    }
    cgmatch(cg, TOK_SEMICOLON);
}

static void cg_input_stmt(CodeGen *cg) {
    if (cgcheck(cg, TOK_IDENT)) {
        emit(cg, TAC_INPUT, cgcur(cg)->lexeme, "", "");
        cgadvance(cg);
    }
    cgmatch(cg, TOK_SEMICOLON);
}

static void cg_output_stmt(CodeGen *cg) {
    char val[MAX_TAC_NAME];
    cg_expr(cg, val);
    emit(cg, TAC_OUTPUT, "", val, "");
    cgmatch(cg, TOK_SEMICOLON);
}

static void cg_statement(CodeGen *cg) {
    if (is_type_tok(cgcur(cg)->type)) {
        cgadvance(cg);
        cg_var_decl(cg);
        return;
    }
    switch (cgcur(cg)->type) {
        case TOK_IF:     cgadvance(cg); cg_if_stmt(cg);     break;
        case TOK_WHILE:  cgadvance(cg); cg_while_stmt(cg);  break;
        case TOK_FOR:    cgadvance(cg); cg_for_stmt(cg);    break;
        case TOK_RETURN: cgadvance(cg); cg_return_stmt(cg); break;
        case TOK_INPUT:  cgadvance(cg); cg_input_stmt(cg);  break;
        case TOK_OUTPUT: cgadvance(cg); cg_output_stmt(cg); break;
        case TOK_RBRACE:
        case TOK_EOF:    break;
        default: {
            char tmp[MAX_TAC_NAME];
            cg_expr(cg, tmp);
            cgmatch(cg, TOK_SEMICOLON);
            break;
        }
    }
}

static void cg_block(CodeGen *cg) {
    cgmatch(cg, TOK_LBRACE);
    while (!cgcheck(cg, TOK_RBRACE) && !cgcheck(cg, TOK_EOF))
        cg_statement(cg);
    cgmatch(cg, TOK_RBRACE);
}

/* ── Function declaration ─────────────────────────────────────────────────── */
static void cg_func_decl(CodeGen *cg) {
    char name[MAX_TAC_NAME];
    strncpy(name, cgcur(cg)->lexeme, MAX_TAC_NAME - 1);
    name[MAX_TAC_NAME-1] = '\0';
    cgadvance(cg); /* IDENT */

    emit(cg, TAC_FUNC_BEGIN, name, "", "");

    cgmatch(cg, TOK_LPAREN);
    /* Parameters – just skip tokens; already validated by semantic */
    while (!cgcheck(cg, TOK_RPAREN) && !cgcheck(cg, TOK_EOF))
        cgadvance(cg);
    cgmatch(cg, TOK_RPAREN);

    cg_block(cg);

    emit(cg, TAC_FUNC_END, name, "", "");
}

/* ── Top-level ────────────────────────────────────────────────────────────── */
static void cg_declaration(CodeGen *cg) {
    if (!is_type_tok(cgcur(cg)->type)) { cgadvance(cg); return; }
    cgadvance(cg); /* consume type */

    if (cgcheck(cg, TOK_IDENT) && cg->pos + 1 < cg->count &&
        cg->tokens[cg->pos + 1].type == TOK_LPAREN) {
        cg_func_decl(cg);
    } else {
        cg_var_decl(cg);
    }
}

/* ── Public API ───────────────────────────────────────────────────────────── */
void codegen_init(CodeGen *cg, Lexer *lex, ErrorHandler *err) {
    cg->tokens     = lex->tokens;
    cg->count      = lex->token_count;
    cg->pos        = 0;
    cg->code_count = 0;
    cg->temp_count = 0;
    cg->label_count= 0;
    cg->err        = err;
}

void codegen_run(CodeGen *cg) {
    printf("[CodeGen] Starting intermediate code generation...\n");
    while (!cgcheck(cg, TOK_EOF))
        cg_declaration(cg);
    printf("[CodeGen] Generated %d TAC instruction(s).\n", cg->code_count);
}

/* ── Pretty print ─────────────────────────────────────────────────────────── */
static const char *op_sym(TACOp op) {
    switch (op) {
        case TAC_ADD: return "+";   case TAC_SUB: return "-";
        case TAC_MUL: return "*";   case TAC_DIV: return "/";
        case TAC_MOD: return "%";   case TAC_AND: return "&&";
        case TAC_OR:  return "||";  case TAC_EQ:  return "==";
        case TAC_NEQ: return "!=";  case TAC_LT:  return "<";
        case TAC_GT:  return ">";   case TAC_LEQ: return "<=";
        case TAC_GEQ: return ">=";
        default: return "?";
    }
}

void codegen_print(CodeGen *cg) {
    printf("\n");
    printf("┌──────────────────────────────────────────────────────────────────────────┐\n");
    printf("│              THREE-ADDRESS INTERMEDIATE CODE (TAC)                      │\n");
    printf("├──────┬───────────────────────────────────────────────────────────────────┤\n");
    printf("│ %-4s │ %-69s│\n", "No.", "Instruction");
    printf("├──────┼───────────────────────────────────────────────────────────────────┤\n");

    for (int i = 0; i < cg->code_count; i++) {
        TACInstr *ins = &cg->code[i];
        char line[128] = "";

        switch (ins->op) {
            case TAC_ASSIGN:
                snprintf(line, sizeof(line), "%s = %s", ins->result, ins->arg1);
                break;
            case TAC_ADD: case TAC_SUB: case TAC_MUL: case TAC_DIV: case TAC_MOD:
            case TAC_AND: case TAC_OR:
            case TAC_EQ:  case TAC_NEQ: case TAC_LT:
            case TAC_GT:  case TAC_LEQ: case TAC_GEQ:
                snprintf(line, sizeof(line), "%s = %s %s %s",
                         ins->result, ins->arg1, op_sym(ins->op), ins->arg2);
                break;
            case TAC_NEG:
                snprintf(line, sizeof(line), "%s = -%s", ins->result, ins->arg1);
                break;
            case TAC_NOT:
                snprintf(line, sizeof(line), "%s = !%s", ins->result, ins->arg1);
                break;
            case TAC_INC:
                snprintf(line, sizeof(line), "%s = %s + 1", ins->result, ins->arg1);
                break;
            case TAC_DEC:
                snprintf(line, sizeof(line), "%s = %s - 1", ins->result, ins->arg1);
                break;
            case TAC_GOTO:
                snprintf(line, sizeof(line), "goto %s", ins->arg1);
                break;
            case TAC_IF_FALSE:
                snprintf(line, sizeof(line), "if !%s goto %s", ins->arg1, ins->arg2);
                break;
            case TAC_LABEL:
                snprintf(line, sizeof(line), "%s:", ins->result);
                break;
            case TAC_PARAM:
                snprintf(line, sizeof(line), "param %s", ins->arg1);
                break;
            case TAC_CALL:
                snprintf(line, sizeof(line), "%s = call %s, %s",
                         ins->result, ins->arg1, ins->arg2);
                break;
            case TAC_RETURN:
                if (ins->arg1[0])
                    snprintf(line, sizeof(line), "return %s", ins->arg1);
                else
                    snprintf(line, sizeof(line), "return");
                break;
            case TAC_INPUT:
                snprintf(line, sizeof(line), "input %s", ins->result);
                break;
            case TAC_OUTPUT:
                snprintf(line, sizeof(line), "output %s", ins->arg1);
                break;
            case TAC_FUNC_BEGIN:
                snprintf(line, sizeof(line), "---- begin %s ----", ins->result);
                break;
            case TAC_FUNC_END:
                snprintf(line, sizeof(line), "---- end %s ----", ins->result);
                break;
        }

        printf("│ %-4d │ %-69s│\n", i, line);
    }

    printf("└──────┴───────────────────────────────────────────────────────────────────┘\n");
}
