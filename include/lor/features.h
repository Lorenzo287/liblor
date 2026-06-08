// SPDX-License-Identifier: MIT

#ifndef LOR_FEATURES_H
#define LOR_FEATURES_H

// Standard C11 generic selection.
#if !defined(__cplusplus) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define LOR_HAS_GENERIC_SELECTION 1
#else
#define LOR_HAS_GENERIC_SELECTION 0
#endif

/* Type declaration helper.

   GCC and Clang provide `__typeof__` in C11 mode. C23 provides standard
   `typeof`. */
#if !defined(__cplusplus) && (defined(__GNUC__) || defined(__clang__))
#define LOR_HAS_TYPEOF 1
#define lor_typeof(value) __typeof__(value)
#elif !defined(__cplusplus) && defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 202311L
#define LOR_HAS_TYPEOF 1
#define lor_typeof(value) typeof(value)
#else
#define LOR_HAS_TYPEOF 0
#endif

// Statement expressions are a separate GCC/Clang extension.
#if !defined(__cplusplus) && (defined(__GNUC__) || defined(__clang__))
#define LOR_HAS_STATEMENT_EXPRESSIONS 1
#else
#define LOR_HAS_STATEMENT_EXPRESSIONS 0
#endif

#endif
