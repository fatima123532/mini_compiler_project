#ifndef CODEGEN_H
#define CODEGEN_H

#include "common.h"
#include "lexer.h"
#include "symbol_table.h"
#include "error.h"

/*
 * Intermediate Code Generation – Three-Address Code (TAC)
 *
 * TAC Instructions:
 *   ASSIGN    :  result = arg1
 *   ADD       :  result = arg1 + arg2
 *   SUB       :  result = arg1 - arg2
 *   MUL       :  result = arg1 * arg2
 *   DIV       :  result = arg1 / arg2
 *   MOD       :  result = arg1 % arg2
 *   NEG       :  result = -arg1
 *   NOT       :  result = !arg1
 *   AND       :  result = arg1 && arg2
 *   OR        :  result = arg1 || arg2
 *   EQ        :  result = arg1 == arg2
 *   NEQ       :  result = arg1 != arg2
 *   LT        :  result = arg1 < arg2
 *   GT        :  result = arg1 > arg2
 *   LEQ       :  result = arg1 <= arg2
 *   GEQ       :  result = arg1 >= arg2
 *   INC       :  result = result + 1
 *   DEC       :  result = result - 1
 *   GOTO      :  goto label
 *   IF_FALSE  :  if !arg1 goto label
 *   LABEL     :  label:
 *   PARAM     :  param arg1         (push argument)
 *   CALL      :  result = call func, n   (n = arg count)
 *   RETURN    :  return arg1
 *   INPUT     :  input result
 *   OUTPUT    :  output arg1
 *   FUNC_BEGIN:  begin func
 *   FUNC_END  :  end func
 */

#define MAX_TAC       2048
#define MAX_TAC_NAME  64

typedef enum {
    TAC_ASSIGN,
    TAC_ADD, TAC_SUB, TAC_MUL, TAC_DIV, TAC_MOD,
    TAC_NEG, TAC_NOT,
    TAC_AND, TAC_OR,
    TAC_EQ, TAC_NEQ, TAC_LT, TAC_GT, TAC_LEQ, TAC_GEQ,
    TAC_INC, TAC_DEC,
    TAC_GOTO,
    TAC_IF_FALSE,
    TAC_LABEL,
    TAC_PARAM,
    TAC_CALL,
    TAC_RETURN,
    TAC_INPUT,
    TAC_OUTPUT,
    TAC_FUNC_BEGIN,
    TAC_FUNC_END
} TACOp;

typedef struct {
    TACOp op;
    char  result[MAX_TAC_NAME];
    char  arg1  [MAX_TAC_NAME];
    char  arg2  [MAX_TAC_NAME];
} TACInstr;

typedef struct {
    Token        *tokens;
    int           count;
    int           pos;
    TACInstr      code[MAX_TAC];
    int           code_count;
    int           temp_count;   /* t0, t1, t2 ... */
    int           label_count;  /* L0, L1, L2 ... */
    ErrorHandler *err;
} CodeGen;

/* ── Public API ─────────────────────────────────────────────────────────── */
void codegen_init   (CodeGen *cg, Lexer *lex, ErrorHandler *err);
void codegen_run    (CodeGen *cg);
void codegen_print  (CodeGen *cg);

#endif /* CODEGEN_H */
