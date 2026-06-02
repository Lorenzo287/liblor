#include <stdarg.h>
#include <stdio.h>

// NOTE: printf with optional end("") as SECOND argument, 
// uses _Generic with an anonymous struct to recognize end sequence

// WARN: since fmt and end_str are both char* _Generic cannot distinguish them
// -> we need to define a pointer to a new type that _Generic can recognize
// (does not matter what type, since to dereference it we cast back to char*)
typedef struct opaque *end_t;
#define end(x) (end_t)(x)

void _print(const char *end_str, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("%s", end_str);
}

// case without arguments
#define print_1(fmt) _print("\n", fmt)

// case with one or more args
#define print_n(fmt, second, ...)                                  \
    _Generic((second),                                             \
        end_t: _print((const char *)(second), fmt, ##__VA_ARGS__), \
        default: _print("\n", fmt, second, ##__VA_ARGS__))

// WARN: macro handles up to 10 args, can be extended manually
#define GET_PRINT_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, NAME, ...) NAME
#define print(...)                                                            \
    GET_PRINT_MACRO(__VA_ARGS__, print_n, print_n, print_n, print_n, print_n, \
                    print_n, print_n, print_n, print_n, print_1)(__VA_ARGS__)

int main(void) {
    char *s = "hello";

    print("|%s with newline|", s);
    print("|%s with custom ending|", end("!\n"), s);
    print("|%d %f|", end(""), 10, 3.14);
    print("|no newline before this|", end(""));
    print("|hello|");

    return 0;
}
