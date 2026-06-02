#include <stdio.h>

// NOTE: variadic print that accepts multiple args without format,
// each data gets dispatched to a dedicated print func based on the data type

void print_int(int x) {
    printf("%d", x);
}
void print_double(double x) {
    printf("%f", x);
}
void print_str(const char *x) {
    printf("%s", x);
}

#define print_one(x)             \
    _Generic((x),                \
        int: print_int,          \
        double: print_double,    \
        const char *: print_str, \
        char *: print_str)(x);   \
    printf(" ");

#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define print(...)                                                      \
    GET_MACRO(__VA_ARGS__, print_8, print_7, print_6, print_5, print_4, \
              print_3, print_2, print_1)(__VA_ARGS__)

#define print_1(a) print_one(a)
#define print_2(a, b) print_one(a) print_one(b)
#define print_3(a, b, c) print_one(a) print_one(b) print_one(c)
#define print_4(a, b, c, d) print_one(a) print_one(b) print_one(c) print_one(d)
#define print_5(a, b, c, d, e) \
    print_one(a) print_one(b) print_one(c) print_one(d) print_one(e)
#define print_6(a, b, c, d, e, f)                                    \
    print_one(a) print_one(b) print_one(c) print_one(d) print_one(e) \
        print_one(f)
#define print_7(a, b, c, d, e, f, g)                                 \
    print_one(a) print_one(b) print_one(c) print_one(d) print_one(e) \
        print_one(f) print_one(g)
#define print_8(a, b, c, d, e, f, g, h)                              \
    print_one(a) print_one(b) print_one(c) print_one(d) print_one(e) \
        print_one(f) print_one(g) print_one(h)

int main() {
    print(10, "hello", 3.14, "world", 42);
    printf("\n");
    print("C", "macros", 123, 4.56);
    printf("\n");
    return 0;
}
