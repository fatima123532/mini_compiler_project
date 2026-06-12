// test_valid.c  –  Valid mini-compiler source program

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

float hypotenuse(float a, float b) {
    return a * a + b * b;
}

bool is_even(int n) {
    int r = n % 2;
    bool result = false;
    if (r == 0) {
        result = true;
    }
    return result;
}

void main() {
    // Variable declarations
    int x = 0;
    int y = 10;
    float pi = 3.14;
    bool done = false;

    // Input / output
    input x;
    output x;

    // if-else
    if (x > 5) {
        y = x * 2;
    } else {
        y = x + 1;
    }

    // while loop
    int count = 0;
    while (count < y) {
        count += 1;
    }

    // for loop with scoped variable
    int total = 0;
    for (int i = 0; i < 10; i++) {
        total += i;
    }

    // Function calls
    int fact = factorial(5);
    bool even = is_even(fact);

    output total;
    output fact;
}
