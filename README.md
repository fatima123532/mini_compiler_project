# Mini-Compiler — CSC303L Compiler Construction
### University of Engineering and Technology, New Campus, Lahore
### Department of Computer Science

---

## What is this project?

This is a **Mini-Compiler** built in C language for a subset of C-like programming language.
It processes source code through **5 complete phases**:

```
Source Code
    ↓
[Phase 1] LEXER        → Breaks code into tokens
    ↓
[Phase 2] PARSER       → Checks grammar/structure
    ↓
[Phase 3] SEMANTIC     → Checks meaning & types
    ↓
[Phase 4] SYMBOL TABLE → Tracks variables & functions
    ↓
[Phase 5] CODE GEN     → Generates Three-Address Code (TAC)
    ↓
Intermediate Code Output
```

---

## Files in this Project

| File | Purpose |
|------|---------|
| `common.h` | Shared types — TokenType, DataType enums, utility functions |
| `error.h / error.c` | Error handler — collects errors from all phases with line numbers |
| `symbol_table.h / symbol_table.c` | Symbol table — tracks variables, functions, scope |
| `lexer.h / lexer.c` | Lexical analyzer — converts source code to tokens |
| `parser.h / parser.c` | Syntax analyzer — validates grammar using recursive descent |
| `semantic.h / semantic.c` | Semantic analyzer — type checking, scope resolution |
| `codegen.h / codegen.c` | Code generator — produces Three-Address Code (TAC) |
| `mini_compiler.c` | Main driver — connects all phases together |
| `test_valid.c` | Test case: Valid program (should compile successfully) |
| `test_invalid.c` | Test case: Invalid program (contains deliberate errors) |
| `Makefile` | Build file for Linux/Unix |

---

## Supported Language Features

### Data Types
- `int` — Integer numbers
- `float` — Decimal numbers
- `bool` — true / false
- `void` — For functions with no return value

### Control Structures
- `if-else` — Conditional execution
- `while` — While loop
- `for` — For loop

### Functions
- Function declaration with parameters
- Return types (int, float, bool, void)
- Recursive function calls

### Statements
- `input` — Read a variable
- `output` — Print a value
- Assignment: `=`, `+=`, `-=`
- Increment/Decrement: `++`, `--`

### Operators
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Relational: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical: `&&`, `||`, `!`

### Comments
- Single-line: `// this is a comment`
- Multi-line: `/* this is a comment */`

---

## How to Compile (Build the Compiler)

### On Windows (PowerShell)

**Step 1 — Set encoding (for proper display):**
```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
```

**Step 2 — Compile all files:**
```powershell
gcc mini_compiler.c lexer.c parser.c semantic.c symbol_table.c error.c codegen.c -o mini_compiler.exe
```

### On Linux / Mac (Terminal)
```bash
make
```
OR manually:
```bash
gcc mini_compiler.c lexer.c parser.c semantic.c symbol_table.c error.c codegen.c -o mini_compiler -Wall
```

---

## How to Run

### 3 Modes Available:

#### Mode 1 — Interactive Mode (type code live)
```powershell
.\mini_compiler.exe
```
- Type your code line by line
- Press ENTER on empty line after last `}` to compile
- Type `exit` to quit

**Example input in interactive mode:**
```
void main() {
    int x = 10;
    int y = 20;
    if (x < y) {
        output x;
    }
}

```
*(press ENTER on empty line to compile)*

---

#### Mode 2 — File Mode (compile a .c file)
```powershell
.\mini_compiler.exe test_valid.c
.\mini_compiler.exe test_invalid.c
.\mini_compiler.exe yourfile.c
```

---

#### Mode 3 — Demo Mode (built-in examples)
```powershell
.\mini_compiler.exe --demo
```
Runs two built-in programs:
1. A valid program — compiles successfully
2. An error program — shows all error types

---

## Test Cases

### test_valid.c — Valid Program
Contains:
- `factorial()` — recursive function (int return type)
- `hypotenuse()` — float return type function
- `is_even()` — bool return type function
- `if-else`, `while` loop, `for` loop
- `input` and `output` statements
- `+=` compound assignment
- Function calls

**Expected result:** Compilation successful ✓

---

### test_invalid.c — Invalid Program
Contains 5 deliberate errors:

| # | Error Type | Code Example | Line |
|---|-----------|--------------|------|
| 1 | Redeclaration | `int x=5; int x=10;` | Line 9 |
| 2 | Type mismatch | `bool flag = 5+3;` | Line 10 |
| 3 | Undeclared variable | `output y;` | Line 11 |
| 4 | Wrong arity | `multiply(1)` expects 2 args | Line 12 |
| 5 | Non-bool condition | `if (x+1)` | Line 14 |

**Expected result:** Compilation failed with 5 error(s) ✗

---

## What the Output Shows

When you run the compiler, you will see:

### 1. Lexer Token Stream Table
```
┌────────┬──────────────────┬───────────────────┐
│ Line   │ Token Type       │ Lexeme            │
├────────┼──────────────────┼───────────────────┤
│ 1      │ void             │ void              │
│ 1      │ IDENT            │ main              │
...
```

### 2. Parser Status
```
[Parser] Syntax analysis completed successfully.
```

### 3. Semantic Status
```
[Semantic] Semantic analysis completed successfully.
```

### 4. Symbol Table
```
┌─────────────────┬──────────┬───────┬───────┬────────────────┐
│ Name            │ Type     │ Kind  │ Scope │ Initialized    │
├─────────────────┼──────────┼───────┼───────┼────────────────┤
│ main            │ void     │ FUNC  │ 0     │ No             │
...
```

### 5. Three-Address Code (TAC)
```
┌──────┬───────────────────────────────────────────┐
│ No.  │ Instruction                               │
├──────┼───────────────────────────────────────────┤
│ 0    │ ---- begin main ----                      │
│ 1    │ x = 10                                    │
│ 2    │ y = 20                                    │
│ 3    │ t0 = x < y                                │
│ 4    │ if !t0 goto L0                            │
│ 5    │ output x                                  │
│ 6    │ L0:                                       │
│ 7    │ ---- end main ----                        │
...
```

### 6. Compilation Summary
```
✓ Compilation successful.
  OR
✗ Compilation failed with 5 error(s).
```

---

## Error Messages Explained

| Error Phase | Meaning | Example |
|------------|---------|---------|
| `[Lexical Error]` | Invalid character in source | Unexpected character `@` |
| `[Syntax Error]` | Wrong code structure | Missing `;` or `}` |
| `[Semantic Error]` | Wrong meaning/types | Using undeclared variable |

---

## Three-Address Code (TAC) Explained

TAC is an intermediate representation where each instruction has at most 3 operands.

| Instruction | Meaning |
|------------|---------|
| `x = 5` | Assign value 5 to x |
| `t0 = a + b` | Add a and b, store in temporary t0 |
| `t1 = x < y` | Compare x and y, result in t1 |
| `if !t1 goto L0` | If t1 is false, jump to label L0 |
| `goto L2` | Unconditional jump to L2 |
| `L0:` | Label — jump target |
| `param x` | Push x as function argument |
| `t2 = call add, 2` | Call function add with 2 args |
| `return t0` | Return value t0 from function |
| `input x` | Read value into x |
| `output t0` | Print value of t0 |
| `---- begin f ----` | Function f starts here |
| `---- end f ----` | Function f ends here |

---

## Grammar Rules (Summary)

```
program         → declaration* EOF
declaration     → func_decl | var_decl
func_decl       → type IDENT '(' params? ')' block
var_decl        → type IDENT ('=' expression)? ';'
block           → '{' statement* '}'
statement       → var_decl | if_stmt | while_stmt | for_stmt
                | return_stmt | input_stmt | output_stmt | expr_stmt
if_stmt         → 'if' '(' expression ')' block ('else' block)?
while_stmt      → 'while' '(' expression ')' block
for_stmt        → 'for' '(' init ';' condition ';' update ')' block
expression      → assignment | logical | relational | arithmetic
type            → 'int' | 'float' | 'bool' | 'void'
```

---

## Quick Viva Reference

**Q: What is a Compiler?**
A: A program that translates source code from one language to another.

**Q: What is a Token?**
A: The smallest meaningful unit of source code (keyword, identifier, operator, literal).

**Q: What does the Lexer do?**
A: Reads characters and groups them into tokens.

**Q: What does the Parser do?**
A: Validates the token stream against grammar rules (checks structure).

**Q: What is Semantic Analysis?**
A: Checks the meaning — type checking, scope resolution, declaration validation.

**Q: What is a Symbol Table?**
A: A data structure that stores information about all identifiers (name, type, scope).

**Q: What is TAC?**
A: Three-Address Code — intermediate representation where each instruction has max 3 operands.

**Q: What is Error Recovery?**
A: After an error, the compiler skips to a safe point and continues to find more errors.

**Q: What is Scope?**
A: The region of code where a variable is visible and accessible.

**Q: What is Recursive Descent Parser?**
A: A parser where each grammar rule has its own C function that calls other functions recursively.

---



*Built with C language — No external compiler-generation tools used.*
