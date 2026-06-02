#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define type(x)                           \
    _Generic((x),                         \
        _Bool: "bool",                    \
        char: "char",                     \
        signed char: "s-char",            \
        unsigned char: "u-char",          \
        short: "short",                   \
        unsigned short: "ushort",         \
        int: "int",                       \
        unsigned int: "uint",             \
        long: "long",                     \
        unsigned long: "ulong",           \
        long long: "long long",           \
        unsigned long long: "ulong long", \
        float: "float",                   \
        double: "double",                 \
        long double: "long double",       \
        char *: "string",                 \
        const char *: "string",           \
        void *: "void pointer",           \
        default: "pointer/other")

#define type_bit(x)                   \
    _Generic((x),                     \
        _Bool: "bool",                \
        char: "char",                 \
        signed char: "int8",          \
        unsigned char: "uint8",       \
        short: "int16",               \
        unsigned short: "uint16",     \
        int: "int32",                 \
        unsigned int: "uint32",       \
        long: "long",                 \
        unsigned long: "ulong",       \
        long long: "int64",           \
        unsigned long long: "uint64", \
        float: "float",               \
        double: "double",             \
        char *: "string",             \
        void *: "pointer",            \
        default: "other")

// there is already a gcc reserved keyword (typeof / __typeof__)
// but it is used to DECLARE variables
#define typesafe_max(a, b)  \
    ({                      \
        typeof(a) _a = (a); \
        typeof(b) _b = (b); \
        _a > _b ? _a : _b;  \
    })

int main(void) {
    unsigned int a = 10;
    long int b = 20;
    float c = 3.14f;

    printf("Literal 5:     %s\n", type(5));
    printf("Variable a:    %s\n", type(a));
    printf("Variable b:    %s\n", type(b));
    printf("Literal 3.14:  %s\n", type(3.14));  // Defaults to double
    printf("Literal float: %s\n", type(c));
    printf("String:        %s\n", type("hello"));
    printf("Boolean:       %s\n", type((bool)true));

    return 0;
}
