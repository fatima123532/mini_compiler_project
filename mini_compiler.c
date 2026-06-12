/*
 * mini_compiler.c  –  Main driver
 *
 * Usage:  ./mini_compiler                 (interactive mode - type code live)
 *         ./mini_compiler <source_file>   (compile a .c file)
 *         ./mini_compiler --demo          (run built-in demo programs)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "error.h"
#include "symbol_table.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"

/* ─── Read entire file into buffer ───────────────────────────────────────── */
static int read_file(const char *path, char *buf, int max_len) {
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "Error: cannot open '%s'\n", path); return 0; }
    int len = (int)fread(buf, 1, max_len - 1, f);
    buf[len] = '\0';
    fclose(f);
    return 1;
}

/* ─── Compile a source string ─────────────────────────────────────────────── */
static void compile(const char *source, const char *label) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║          MINI COMPILER  –  %s\n", label);
    printf("╚══════════════════════════════════════════════════════════════╝\n");

    /* ── Phase 1: Lexical Analysis ── */
    ErrorHandler err;
    err_init(&err);

    Lexer lex;
    lexer_init(&lex, source, &err);

    printf("\n[Lexer] Starting lexical analysis...\n");
    lexer_tokenize(&lex);
    printf("[Lexer] Tokenization complete. %d token(s) produced.\n",
           lex.token_count);
    lexer_print(&lex);

    if (err_count(&err) > 0) {
        printf("\n── Lexical Errors ──");
        err_print(&err);
        printf("\nCompilation stopped due to lexical errors.\n");
        return;
    }

    /* ── Phase 2: Syntax Analysis ── */
    Parser parser;
    parser_init(&parser, &lex, &err);
    parser_parse(&parser);

    if (err_count(&err) > 0) {
        printf("\n── Syntax Errors ──");
        err_print(&err);
        printf("\nCompilation stopped due to syntax errors.\n");
        return;
    }

    /* ── Phase 3: Semantic Analysis ── */
    SymbolTable sym;
    sym_init(&sym);

    Semantic sem;
    semantic_init(&sem, &lex, &sym, &err);
    semantic_analyze(&sem);

    /* ── Symbol Table ── */
    sym_print(&sym);

    /* ── Phase 4: Intermediate Code Generation ── */
    if (err_count(&err) == 0) {
        CodeGen cg;
        codegen_init(&cg, &lex, &err);
        codegen_run(&cg);
        codegen_print(&cg);
    } else {
        printf("\n[CodeGen] Skipped due to previous errors.\n");
    }

    /* ── Error Summary ── */
    printf("\n── Compilation Summary ──");
    err_print(&err);

    if (err_count(&err) == 0)
        printf("\n  ✓ Compilation successful.\n");
    else
        printf("\n  ✗ Compilation failed with %d error(s).\n", err_count(&err));
}

/* ─── Built-in demo programs ──────────────────────────────────────────────── */
static const char *demo_valid =
    "// Demo 1: Valid program\n"
    "\n"
    "int add(int a, int b) {\n"
    "    return a + b;\n"
    "}\n"
    "\n"
    "float average(float x, float y) {\n"
    "    return (x + y) / 2.0;\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    int result = 0;\n"
    "    float avg;\n"
    "    bool flag = true;\n"
    "\n"
    "    // Input and output\n"
    "    input result;\n"
    "    output result;\n"
    "\n"
    "    // if-else\n"
    "    if (result > 10) {\n"
    "        flag = false;\n"
    "    } else {\n"
    "        flag = true;\n"
    "    }\n"
    "\n"
    "    // while loop\n"
    "    int i = 0;\n"
    "    while (i < 5) {\n"
    "        i++;\n"
    "    }\n"
    "\n"
    "    // for loop\n"
    "    int sum = 0;\n"
    "    for (int j = 0; j < 10; j++) {\n"
    "        sum += j;\n"
    "    }\n"
    "\n"
    "    // function call\n"
    "    int total = add(result, sum);\n"
    "    output total;\n"
    "}\n";

static const char *demo_errors =
    "// Demo 2: Program with semantic errors\n"
    "\n"
    "int multiply(int a, int b) {\n"
    "    return a * b;\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    int x = 5;\n"
    "    int x = 10;          // ERROR: redeclaration\n"
    "    bool flag = 5 + 3;   // ERROR: type mismatch (int -> bool)\n"
    "    output y;            // ERROR: undeclared variable\n"
    "    int r = multiply(1); // ERROR: wrong arity (expects 2)\n"
    "\n"
    "    if (x + 1) {         // ERROR: if condition not bool\n"
    "        x = 1;\n"
    "    }\n"
    "}\n";

/* ─── Interactive mode: user types code line by line ─────────────────────── */
static void interactive_mode(void) {
    static char source[65536];
    char line[1024];
    int  source_len = 0;
    int  brace_depth = 0;
    int  started = 0;

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║           MINI COMPILER  –  INTERACTIVE MODE               ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  Type your C-like source code below.                        ║\n");
    printf("║  Press ENTER on an empty line after closing brace '}'       ║\n");
    printf("║  to compile. Type 'exit' to quit.                           ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");

    while (1) {
        source_len  = 0;
        brace_depth = 0;
        started     = 0;
        source[0]   = '\0';

        printf("\n>> Enter code (empty line after last '}' to compile):\n");

        while (1) {
            printf("... ");
            fflush(stdout);

            if (!fgets(line, sizeof(line), stdin)) break;

            /* Remove trailing newline */
            int len = (int)strlen(line);
            if (len > 0 && line[len-1] == '\n') line[--len] = '\0';

            /* Exit command */
            if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
                printf("\nGoodbye!\n");
                return;
            }

            /* Empty line: compile if we have content and braces are closed */
            if (len == 0) {
                if (started && brace_depth == 0)
                    break;          /* trigger compilation */
                else if (!started)
                    continue;       /* skip leading blank lines */
                else {
                    /* inside a block — add blank line to source */
                    source[source_len++] = '\n';
                    source[source_len]   = '\0';
                    continue;
                }
            }

            started = 1;

            /* Count braces to know when program is complete */
            for (int i = 0; i < len; i++) {
                if (line[i] == '{') brace_depth++;
                if (line[i] == '}') brace_depth--;
            }

            /* Append line to source buffer */
            if (source_len + len + 2 < 65536) {
                memcpy(source + source_len, line, len);
                source_len += len;
                source[source_len++] = '\n';
                source[source_len]   = '\0';
            } else {
                printf("[Error] Source too long (max 64KB).\n");
                break;
            }

            /* Auto-compile when all braces are closed */
            if (started && brace_depth == 0 && len > 0 && line[len-1] == '}') {
                printf("... [braces balanced - press ENTER to compile or keep typing]\n");
            }
        }

        if (source_len == 0) continue;

        compile(source, "INTERACTIVE INPUT");

        printf("\n──────────────────────────────────────────────────────────────\n");
        printf("  Compile again? (yes/no): ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;
        /* Remove trailing newline */
        int l = (int)strlen(line);
        if (l > 0 && line[l-1] == '\n') line[--l] = '\0';

        if (strcmp(line, "no") == 0 || strcmp(line, "n") == 0 ||
            strcmp(line, "exit") == 0) {
            printf("\nGoodbye!\n");
            break;
        }
    }
}

/* ─── Main ────────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {

    /* --demo flag */
    if (argc == 2 && strcmp(argv[1], "--demo") == 0) {
        compile(demo_valid,  "VALID PROGRAM");
        compile(demo_errors, "PROGRAM WITH ERRORS");
        return 0;
    }

    /* File argument */
    if (argc == 2) {
        char source[65536];
        if (!read_file(argv[1], source, sizeof(source))) return 1;
        compile(source, argv[1]);
        return 0;
    }

    /* No argument → interactive mode */
    if (argc == 1) {
        interactive_mode();
        return 0;
    }

    printf("\nUsage:\n");
    printf("  mini_compiler.exe                  Interactive mode (type code live)\n");
    printf("  mini_compiler.exe <file.c>         Compile a source file\n");
    printf("  mini_compiler.exe --demo           Run built-in demo programs\n");
    return 0;
}
