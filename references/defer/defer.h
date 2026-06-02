/*
A macro to define cleanup functions and variables.
This macro uses GCC's __attribute__((__cleanup__)) extension to
execute a block of code when a variable goes out of scope.

It works similarly to 'defer' statements in other languages like Go.

Usage:
defer { code to be executed at the end of the scope }
*/

#ifndef DEFER_H
#define DEFER_H

#if !defined(__GNUC__) || defined(__clang__)
    #include <errno.h>
    #include <stdio.h>
    #define __DEFER__(F, V)                                                \
        errno = ENOTSUP;                                                   \
        fprintf(stderr,                                                    \
                "Error: 'defer' is only supported with GCC\n  at %s:%d\n", \
                __FILE__, __LINE__);                                       \
        if (0)
#else
    #define __DEFER__(F, V)                    \
        __extension__ auto void F(int *);      \
        int V __attribute__((__cleanup__(F))); \
        __extension__ auto void F(int *)
#endif

#define defer __DEFER(__COUNTER__)
#define __DEFER(N) __DEFER_(N)
#define __DEFER_(N) __DEFER__(__DEFER_FUNCTION_##N, __DEFER_VARIABLE_##N)

#endif  // DEFER_H
