#include "error.h"
#include <stdarg.h>

void err_init(ErrorHandler *eh) {
    eh->count     = 0;
    eh->has_fatal = 0;
}

void err_report(ErrorHandler *eh, ErrorPhase phase, int line,
                const char *fmt, ...) {
    if (eh->count >= MAX_ERRORS) {
        fprintf(stderr, "[ERROR] Too many errors, aborting.\n");
        eh->has_fatal = 1;
        return;
    }

    CompileError *e = &eh->errors[eh->count++];
    e->phase = phase;
    e->line  = line;

    va_list args;
    va_start(args, fmt);
    vsnprintf(e->message, sizeof(e->message), fmt, args);
    va_end(args);
}

void err_print(ErrorHandler *eh) {
    const char *phase_names[] = { "Lexical", "Syntax", "Semantic" };
    printf("\n");
    if (eh->count == 0) {
        printf("  No errors detected.\n");
        return;
    }
    for (int i = 0; i < eh->count; i++) {
        CompileError *e = &eh->errors[i];
        printf("  [%s Error] Line %d: %s\n",
               phase_names[e->phase], e->line, e->message);
    }
    printf("\n  Total: %d error(s)\n", eh->count);
}

int err_count(ErrorHandler *eh) {
    return eh->count;
}
