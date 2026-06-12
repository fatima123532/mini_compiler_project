#include "symbol_table.h"
#include "error.h"

void sym_init(SymbolTable *st) {
    st->count         = 0;
    st->current_scope = 0;
    st->scope_top     = 0;
    st->scope_stack[st->scope_top] = 0;
}

void sym_enter_scope(SymbolTable *st) {
    st->current_scope++;
    if (st->scope_top + 1 < MAX_SCOPES)
        st->scope_stack[++st->scope_top] = st->current_scope;
}

void sym_exit_scope(SymbolTable *st) {
    /* Remove all symbols belonging to the exiting scope */
    int leaving = st->scope_stack[st->scope_top];
    int new_count = 0;
    for (int i = 0; i < st->count; i++) {
        if (st->entries[i].scope_level < leaving) {
            if (new_count != i)
                st->entries[new_count] = st->entries[i];
            new_count++;
        }
    }
    st->count = new_count;

    if (st->scope_top > 0)
        st->scope_top--;
    st->current_scope = st->scope_stack[st->scope_top];
}

/* Returns 1 on success, 0 on duplicate */
int sym_insert(SymbolTable *st, const char *name, DataType dtype,
               SymbolKind kind, int line) {
    (void)line; /* used by callers for error reporting */

    /* Check for duplicate in current scope */
    for (int i = 0; i < st->count; i++) {
        if (st->entries[i].scope_level == st->current_scope &&
            strcmp(st->entries[i].name, name) == 0) {
            return 0; /* duplicate */
        }
    }

    if (st->count >= MAX_SYMBOLS) return 0;

    Symbol *s         = &st->entries[st->count++];
    strncpy(s->name, name, MAX_NAME_LEN - 1);
    s->name[MAX_NAME_LEN - 1] = '\0';
    s->dtype          = dtype;
    s->kind           = kind;
    s->scope_level    = st->current_scope;
    s->is_initialized = 0;
    s->param_count    = 0;
    return 1;
}

/* Searches from innermost scope outward */
Symbol *sym_lookup(SymbolTable *st, const char *name) {
    /* walk backwards to prefer inner-scope bindings */
    for (int i = st->count - 1; i >= 0; i--) {
        if (strcmp(st->entries[i].name, name) == 0)
            return &st->entries[i];
    }
    return NULL;
}

Symbol *sym_lookup_current_scope(SymbolTable *st, const char *name) {
    for (int i = st->count - 1; i >= 0; i--) {
        if (st->entries[i].scope_level == st->current_scope &&
            strcmp(st->entries[i].name, name) == 0)
            return &st->entries[i];
    }
    return NULL;
}

void sym_print(SymbolTable *st) {
    const char *kind_str[] = { "VAR", "FUNC", "PARAM" };
    printf("\n┌──────────────────────────────────────────────────────────────┐\n");
    printf("│                        SYMBOL TABLE                         │\n");
    printf("├─────────────────┬──────────┬───────┬───────┬────────────────┤\n");
    printf("│ %-15s │ %-8s │ %-5s │ %-5s │ %-14s │\n",
           "Name", "Type", "Kind", "Scope", "Initialized");
    printf("├─────────────────┼──────────┼───────┼───────┼────────────────┤\n");
    for (int i = 0; i < st->count; i++) {
        Symbol *s = &st->entries[i];
        printf("│ %-15s │ %-8s │ %-5s │ %-5d │ %-14s │\n",
               s->name,
               datatype_str(s->dtype),
               kind_str[s->kind],
               s->scope_level,
               s->is_initialized ? "Yes" : "No");
    }
    printf("└─────────────────┴──────────┴───────┴───────┴────────────────┘\n");
}
