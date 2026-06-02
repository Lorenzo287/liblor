#ifndef PRINT_H
#define PRINT_H

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// NOTE: combines the print with variadic args (without format) and end(""),
// the end sequence can be passed in any position

/* Custom type to mark the terminator */
typedef struct {
    const char *s;
} end_t;
#define end(x) \
    (end_t) {  \
        x      \
    }

typedef signed char schar_t;
typedef unsigned char uchar_t;
typedef unsigned short ushort_t;
typedef unsigned int uint_t;
typedef unsigned long ulong_t;
typedef long long llong_t;
typedef unsigned long long ullong_t;
typedef long double ldouble_t;
typedef char *char_p_t;
typedef const char *const_char_p_t;
typedef void *void_p_t;

/* Expanded Type mapping */
#define TYPE_LIST(V)                      \
    V(_Bool, bool, "%d")                  \
    V(char, char, "%c")                   \
    V(schar_t, schar, "%d")               \
    V(uchar_t, uchar, "%u")               \
    V(short, short, "%d")                 \
    V(ushort_t, ushort, "%u")             \
    V(int, int, "%d")                     \
    V(uint_t, uint, "%u")                 \
    V(long, long, "%ld")                  \
    V(ulong_t, ulong, "%lu")              \
    V(llong_t, llong, "%lld")             \
    V(ullong_t, ullong, "%llu")           \
    V(float, float, "%g")                 \
    V(double, double, "%g")               \
    V(ldouble_t, ldouble, "%Lg")          \
    V(char_p_t, char_p, "%s")             \
    V(const_char_p_t, const_char_p, "%s") \
    V(void_p_t, void_p, "%p")

/* Printer functions */
#define DEF_PRINTER(TYPE, SUFFIX, FMT)                                \
    static inline void _pr_##SUFFIX(TYPE x, const char **e, int *s) { \
        if (*s) printf(" ");                                          \
        printf(FMT, x);                                               \
        *s = 1;                                                       \
        (void)e;                                                      \
    }
TYPE_LIST(DEF_PRINTER)

static inline void _pr_end_t(end_t x, const char **e, int *s) {
    *e = x.s;
    (void)s;
}

#define CASE_PRINTER(TYPE, SUFFIX, FMT) \
TYPE:                                   \
    _pr_##SUFFIX,

/* The Dispatcher */
#define PRINT_ONE(x, e_ptr, s_ptr)                \
    _Generic((x),                                 \
        TYPE_LIST(CASE_PRINTER) end_t: _pr_end_t, \
        default: _pr_void_p)(x, e_ptr, s_ptr)

/* Macro Magic to handle variable arguments (up to 16) */
#define _PR_1(a) PRINT_ONE(a, &_e, &_s);
#define _PR_2(a, ...)       \
    PRINT_ONE(a, &_e, &_s); \
    _PR_1(__VA_ARGS__)
#define _PR_3(a, ...)       \
    PRINT_ONE(a, &_e, &_s); \
    _PR_2(__VA_ARGS__)
#define _PR_4(a, ...)       \
    PRINT_ONE(a, &_e, &_s); \
    _PR_3(__VA_ARGS__)
#define _PR_5(a, ...)       \
    PRINT_ONE(a, &_e, &_s); \
    _PR_4(__VA_ARGS__)
#define _PR_6(a, ...)       \
    PRINT_ONE(a, &_e, &_s); \
    _PR_5(__VA_ARGS__)
#define _PR_7(a, ...)       \
    PRINT_ONE(a, &_e, &_s); \
    _PR_6(__VA_ARGS__)
#define _PR_8(a, ...)       \
    PRINT_ONE(a, &_e, &_s); \
    _PR_7(__VA_ARGS__)
#define _PR_9(a, ...)       \
    PRINT_ONE(a, &_e, &_s); \
    _PR_8(__VA_ARGS__)
#define _PR_10(a, ...)      \
    PRINT_ONE(a, &_e, &_s); \
    _PR_9(__VA_ARGS__)
#define _PR_11(a, ...)      \
    PRINT_ONE(a, &_e, &_s); \
    _PR_10(__VA_ARGS__)
#define _PR_12(a, ...)      \
    PRINT_ONE(a, &_e, &_s); \
    _PR_11(__VA_ARGS__)
#define _PR_13(a, ...)      \
    PRINT_ONE(a, &_e, &_s); \
    _PR_12(__VA_ARGS__)
#define _PR_14(a, ...)      \
    PRINT_ONE(a, &_e, &_s); \
    _PR_13(__VA_ARGS__)
#define _PR_15(a, ...)      \
    PRINT_ONE(a, &_e, &_s); \
    _PR_14(__VA_ARGS__)
#define _PR_16(a, ...)      \
    PRINT_ONE(a, &_e, &_s); \
    _PR_15(__VA_ARGS__)

#define _GET_PR(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, \
                _15, _16, NAME, ...)                                         \
    NAME
#define print(...)                                                           \
    do {                                                                     \
        const char *_e = "\n";                                               \
        int _s = 0;                                                          \
        _GET_PR(__VA_ARGS__, _PR_16, _PR_15, _PR_14, _PR_13, _PR_12, _PR_11, \
                _PR_10, _PR_9, _PR_8, _PR_7, _PR_6, _PR_5, _PR_4, _PR_3,     \
                _PR_2, _PR_1)(__VA_ARGS__) printf("%s", _e);                 \
    } while (0)

int main(void) {
    /* Test 1: Standard printing (should have NO trailing space before \n) */
    printf("Test 1: [");
    print(10, "hello", 3.140000, "world");
    printf("]\n");

    /* Test 2: Custom terminator (should be exactly "world!") */
    printf("Test 2: [");
    print(end("!"), "hello", "world");
    printf("]\n");

    /* Test 3: Multiple data types */
    void *ptr = (void *)0xDEADBEEF;
    typedef struct undef *undef_t;
    undef_t var = ptr;
    print(true, 'A', 100UL, var, 0.000001);

    /* Test 4: end() in the middle */
    print(1, 2, end(" | "), 3, 4);
    return 0;
}

#endif
