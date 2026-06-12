#ifndef ERROR_H
#define ERROR_H

#include "common.h"

#define MAX_ERRORS 128

typedef enum {
    ERR_LEXICAL,
    ERR_SYNTAX,
    ERR_SEMANTIC
} ErrorPhase;

typedef struct {
    ErrorPhase phase;
    int        line;
    char       message[512];
} CompileError;

typedef struct {
    CompileError errors[MAX_ERRORS];
    int          count;
    int          has_fatal;
} ErrorHandler;

void err_init   (ErrorHandler *eh);
void err_report (ErrorHandler *eh, ErrorPhase phase, int line,
                 const char *fmt, ...);
void err_print  (ErrorHandler *eh);
int  err_count  (ErrorHandler *eh);

#endif /* ERROR_H */
