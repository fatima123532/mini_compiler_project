// test_invalid.c  –  Program containing deliberate errors

int add(int a, int b) {
    return a + b;
}

void compute() {
    // Error 1: Use of undeclared variable
    output z;

    // Error 2: Redeclaration in same scope
    int val = 5;
    int val = 10;

    // Error 3: Type mismatch in assignment
    bool flag = val + 1;

    // Error 4: Wrong arity in function call
    int r = add(1, 2, 3);

    // Error 5: Non-bool while condition
    while (val + 1) {
        val -= 1;
    }

    // Error 6: Type mismatch in equality
    if (val == flag) {
        val = 0;
    }
}
