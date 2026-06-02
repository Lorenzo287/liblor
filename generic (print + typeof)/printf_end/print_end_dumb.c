#include <stdarg.h>
#include <stdio.h>

// NOTE: THIS IS DUMB AND USELESS, see WARN 

void _print(const char *end_str, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("%s", end_str);
}

// marker
#define end(x) MARKER, x

// detect END(...)
#define CAT(a, b) a##b
#define IS_MARKER(x) CAT(IS_MARKER_, x)
#define IS_MARKER_MARKER 1
#define IS_MARKER_s 0   // fallback
#define IS_MARKER_10 0  // fallback
// WARN: need to define infinitely many fallbacks

// branching
#define IF(c) CAT(IF_, c)
#define IF_0(t, f) f
#define IF_1(t, f) t

// extract args
#define GET_FIRST(first, ...) first
#define GET_END(marker, end, ...) end
#define DROP_END(marker, end, args...) args

// implementations
#define PRINT_END(fmt, args...) _print(GET_END(args), fmt, DROP_END(args))
#define PRINT_DEFAULT(fmt, args...) _print("\n", fmt, args)

// dispatcher
#define print(fmt, args...) \
    IF(IS_MARKER(GET_FIRST(args)))(PRINT_END, PRINT_DEFAULT)(fmt, args)

int main(void) {
    char *s = "hello";

    print("|%s with newline|", s);
    print("|%s with custom ending|", end("!\n"), s);
    print("|%d %f|", end(""), 10, 3.14);
    // print("|no newline before this|", end(""));

    return 0;
}
