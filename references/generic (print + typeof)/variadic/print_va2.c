#include <stdio.h>
#include <stdbool.h>

// NOTE: variadic print that accepts multiple args without format,
// the format is automatically dispatched based on the data type

#define fmt(x)                       \
    _Generic((x),                    \
        _Bool: "%d ",                \
        char: "%c ",                 \
        signed char: "%d ",          \
        unsigned char: "%u ",        \
        short: "%d ",                \
        unsigned short: "%u ",       \
        int: "%d ",                  \
        unsigned int: "%u ",         \
        long: "%ld ",                \
        unsigned long: "%lu ",       \
        long long: "%lld ",          \
        unsigned long long: "%llu ", \
        float: "%f ",                \
        double: "%lf ",              \
        long double: "%Lf ",         \
        char *: "%s ",               \
        const char *: "%s ",         \
        void *: "%p ",               \
        default: "undefined:%p ")

#define print_one(x) printf(fmt(x), x)

#define print_1(a) \
    do { print_one(a); } while (0)
#define print_2(a, b) \
    do {              \
        print_one(a); \
        print_one(b); \
        printf("\n"); \
    } while (0)
#define print_3(a, b, c) \
    do {                 \
        print_one(a);    \
        print_one(b);    \
        print_one(c);    \
        printf("\n");    \
    } while (0)
#define print_4(a, b, c, d) \
    do {                    \
        print_one(a);       \
        print_one(b);       \
        print_one(c);       \
        print_one(d);       \
        printf("\n");       \
    } while (0)
#define print_5(a, b, c, d, e) \
    do {                       \
        print_one(a);          \
        print_one(b);          \
        print_one(c);          \
        print_one(d);          \
        print_one(e);          \
        printf("\n");          \
    } while (0)
#define print_6(a, b, c, d, e, f) \
    do {                          \
        print_one(a);             \
        print_one(b);             \
        print_one(c);             \
        print_one(d);             \
        print_one(e);             \
        print_one(f);             \
        printf("\n");             \
    } while (0)

#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define print(...)                                                      \
    GET_MACRO(__VA_ARGS__, print_8, print_7, print_6, print_5, print_4, \
              print_3, print_2, print_1)(__VA_ARGS__)

typedef struct undef undefined_t;

int main(void) {
    undefined_t *var;
    print(10, "hello", 3.14, "world", 42);
    print("C", "macros", 123, 4.56);
    print((bool)true, 'A', 3.14f, 100UL, var);
    return 0;
}
