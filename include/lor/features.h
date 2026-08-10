// SPDX-License-Identifier: MIT

#ifndef LOR_FEATURES_H
#define LOR_FEATURES_H

/* C11 generic selection.

   TCC implements `_Generic` while reporting C99 through `__STDC_VERSION__`. */
#if !defined(__cplusplus) && \
    ((defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || \
     defined(__TINYC__))
#define LOR_HAS_GENERIC_SELECTION 1
#else
#define LOR_HAS_GENERIC_SELECTION 0
#endif

/* Windows TCC before 0.9.28 treats `long double` as compatible with `double`,
   so listing both in one generic association is a constraint violation. */
#if defined(__TINYC__) && __TINYC__ < 928 && defined(_WIN32)
#define LOR_GENERIC_LONG_DOUBLE_CASE(result)
#else
#define LOR_GENERIC_LONG_DOUBLE_CASE(result) long double: result,
#endif

/* GCC, Clang, and current TCC implement the GNU scope cleanup attribute.
   TCC 0.9.27 accepts the syntax but does not run the cleanup function. */
#if defined(__GNUC__) || defined(__clang__) || \
    (defined(__TINYC__) && __TINYC__ >= 928)
#define LOR_HAS_CLEANUP_ATTRIBUTE 1
#else
#define LOR_HAS_CLEANUP_ATTRIBUTE 0
#endif

/* Native MSVC advertises C11 but its C runtime does not declare max_align_t.
   Internal allocation headers use a scalar-type fallback in that case. */
#if defined(_MSC_VER) && !defined(__clang__)
#define LOR_HAS_MAX_ALIGN_T 0
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define LOR_HAS_MAX_ALIGN_T 1
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define LOR_HAS_MAX_ALIGN_T 1
#else
#define LOR_HAS_MAX_ALIGN_T 0
#endif

// C99 compound literals are not part of C++.
#if !defined(__cplusplus) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define LOR_HAS_COMPOUND_LITERALS 1
#else
#define LOR_HAS_COMPOUND_LITERALS 0
#endif

/* Type declaration helper.

   GCC and Clang provide `__typeof__`, MSVC provides it starting with version
   19.39, TCC provides GNU `typeof`, and C23 provides standard `typeof`. */
#if !defined(__cplusplus) && (defined(__GNUC__) || defined(__clang__))
#define LOR_HAS_TYPEOF 1
#define lor_typeof(value) __typeof__(value)
#elif !defined(__cplusplus) && defined(_MSC_VER) && _MSC_VER >= 1939
#define LOR_HAS_TYPEOF 1
#define lor_typeof(value) __typeof__(value)
#elif !defined(__cplusplus) && defined(__TINYC__)
#define LOR_HAS_TYPEOF 1
#define lor_typeof(value) typeof(value)
#elif !defined(__cplusplus) && defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 202311L
#define LOR_HAS_TYPEOF 1
#define lor_typeof(value) typeof(value)
#else
#define LOR_HAS_TYPEOF 0
#endif

// Statement expressions are a separate GCC, Clang, and TCC extension.
#if !defined(__cplusplus) && \
    (defined(__GNUC__) || defined(__clang__) || defined(__TINYC__))
#define LOR_HAS_STATEMENT_EXPRESSIONS 1
#else
#define LOR_HAS_STATEMENT_EXPRESSIONS 0
#endif

#endif
