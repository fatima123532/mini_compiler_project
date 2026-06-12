#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "common.h"

#define MAX_SYMBOLS   512
#define MAX_SCOPES    64
#define MAX_PARAMS    16
#define MAX_NAME_LEN  256

/* ─── Symbol kinds ────────────────────────────────────────────────────────── */
typedef enum {
    SYM_VAR,
    SYM_FUNC,
    SYM_PARAM
} SymbolKind;

/* ─── Single symbol entry ─────────────────────────────────────────────────── */
typedef struct {
    char       name[MAX_NAME_LEN];
    DataType   dtype;          /* variable / return type          */
    SymbolKind kind;
    int        scope_level;
    int        is_initialized;

    /* function-specific */
    int        param_count;
    DataType   param_types[MAX_PARAMS];
} Symbol;

/* ─── Symbol table ────────────────────────────────────────────────────────── */
typedef struct {
    Symbol entries[MAX_SYMBOLS];
    int    count;
    int    current_scope;
    int    scope_stack[MAX_SCOPES];
    int    scope_top;
} SymbolTable;

/* ─── API ─────────────────────────────────────────────────────────────────── */
void    sym_init       (SymbolTable *st);
void    sym_enter_scope(SymbolTable *st);
void    sym_exit_scope (SymbolTable *st);
int     sym_insert     (SymbolTable *st, const char *name, DataType dtype,
                        SymbolKind kind, int line);
Symbol *sym_lookup     (SymbolTable *st, const char *name);
Symbol *sym_lookup_current_scope(SymbolTable *st, const char *name);
void    sym_print      (SymbolTable *st);

#endif /* SYMBOL_TABLE_H */
