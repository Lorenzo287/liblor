// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS

#include "lor/string.h"

#include "lor/memory.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr)                                                          \
    do {                                                                     \
        if (!(expr)) {                                                       \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #expr);                                                  \
            return 1;                                                        \
        }                                                                    \
    } while (0)

#define SV(text) ((LorStringView)LOR_SV_LITERAL(text))

static int test_status_names(void) {
    CHECK(strcmp(lor_status_name(LOR_STATUS_OK), "ok") == 0);
    CHECK(strcmp(lor_status_name(LOR_STATUS_INVALID_ARGUMENT),
                 "invalid argument") == 0);
    CHECK(strcmp(lor_status_name(LOR_STATUS_OUT_OF_MEMORY), "out of memory") ==
          0);
    CHECK(strcmp(lor_status_name(LOR_STATUS_OVERFLOW), "overflow") == 0);
    CHECK(strcmp(lor_status_name((LorStatus)99), "unknown") == 0);
    return 0;
}

static int test_view_construction_and_comparison(void) {
    LorStringView empty = LOR_STRING_VIEW_INIT;
    LorStringView literal = SV("liblor");
    const char binary[] = {'a', '\0', 'b'};

    CHECK(lor_sv_is_valid(empty));
    CHECK(lor_sv_is_empty(empty));
    CHECK(lor_sv_equal(empty, lor_sv_from_cstr(NULL)));
    CHECK(lor_sv_equal(literal, lor_sv_from_cstr("liblor")));
    CHECK(!lor_sv_equal(literal, SV("lib")));
    CHECK(lor_sv_equal(lor_sv_from_parts(binary, sizeof(binary)),
                       lor_sv_from_parts("a\0b", 3)));
    CHECK(lor_sv_is_empty(lor_sv_from_parts(NULL, 3)));
    CHECK(lor_sv_starts_with(literal, SV("lib")));
    CHECK(lor_sv_ends_with(literal, SV("lor")));
    CHECK(!lor_sv_starts_with(literal, SV("lor")));
    CHECK(lor_sv_starts_with(literal, empty));
    CHECK(lor_sv_ends_with(literal, empty));
    return 0;
}

static int test_view_trim_and_slice(void) {
    LorStringView padded = SV(" \t\r\nliblor\v\f ");
    CHECK(lor_sv_equal(lor_sv_trim(padded), SV("liblor")));
    CHECK(lor_sv_equal(lor_sv_trim_left(SV("  left  ")), SV("left  ")));
    CHECK(lor_sv_equal(lor_sv_trim_right(SV("  right  ")), SV("  right")));

    LorStringView text = SV("abcdef");
    CHECK(lor_sv_equal(lor_sv_slice(text, 2, 3), SV("cde")));
    CHECK(lor_sv_equal(lor_sv_slice(text, 4, 99), SV("ef")));
    CHECK(lor_sv_is_empty(lor_sv_slice(text, 99, 2)));
    CHECK(lor_sv_equal(lor_sv_take_left(text, 2), SV("ab")));
    CHECK(lor_sv_equal(lor_sv_take_right(text, 2), SV("ef")));

    LorStringView input = text;
    CHECK(lor_sv_equal(lor_sv_chop_left(&input, 2), SV("ab")));
    CHECK(lor_sv_equal(input, SV("cdef")));
    CHECK(lor_sv_equal(lor_sv_chop_right(&input, 3), SV("def")));
    CHECK(lor_sv_equal(input, SV("c")));
    CHECK(lor_sv_equal(lor_sv_chop_left(&input, 99), SV("c")));
    CHECK(lor_sv_is_empty(input));
    return 0;
}

static int test_view_find_split_and_chop(void) {
    LorStringView text = SV("one::two::three");
    size_t index = 99;

    CHECK(lor_sv_find(text, SV("::three"), &index));
    CHECK(index == 8);
    CHECK(lor_sv_find(text, (LorStringView)LOR_STRING_VIEW_INIT, &index));
    CHECK(index == 0);
    CHECK(lor_sv_find_char(text, 't', &index));
    CHECK(index == 5);
    index = 99;
    CHECK(!lor_sv_find(text, SV("missing"), &index));
    CHECK(index == 99);

    LorStringView before;
    LorStringView after;
    CHECK(lor_sv_split_once(text, SV("::"), &before, &after));
    CHECK(lor_sv_equal(before, SV("one")));
    CHECK(lor_sv_equal(after, SV("two::three")));

    CHECK(!lor_sv_split_once(text, SV("--"), &before, &after));
    CHECK(lor_sv_equal(before, text));
    CHECK(lor_sv_is_empty(after));
    CHECK(!lor_sv_split_once(text, (LorStringView)LOR_STRING_VIEW_INIT, &before,
                             &after));
    CHECK(lor_sv_equal(before, text));

    LorStringView input = SV("a,,b");
    LorStringView part;
    CHECK(lor_sv_chop_char(&input, ',', &part));
    CHECK(lor_sv_equal(part, SV("a")));
    CHECK(lor_sv_chop_char(&input, ',', &part));
    CHECK(lor_sv_is_empty(part));
    CHECK(!lor_sv_chop_char(&input, ',', &part));
    CHECK(lor_sv_equal(part, SV("b")));
    CHECK(lor_sv_is_empty(input));
    return 0;
}

static int test_view_prints_exact_bytes(void) {
    const char bytes[] = {'a', '\0', 'b', ',', 'x'};
    const char *path = ".build/lor_sv_print.bin";
    FILE *file = fopen(path, "wb+");
    CHECK(file != NULL);

    CHECK(lor_sv_fprint(file, lor_sv_from_parts(bytes, 3)));
    CHECK(lor_sv_fprint(file, (LorStringView)LOR_STRING_VIEW_INIT));
    CHECK(!lor_sv_fprint(NULL, SV("x")));
    CHECK(fflush(file) == 0);
    CHECK(fseek(file, 0, SEEK_SET) == 0);

    char actual[4] = {0};
    CHECK(fread(actual, 1, sizeof(actual), file) == 3u);
    CHECK(memcmp(actual, bytes, 3) == 0);
    CHECK(fclose(file) == 0);
    CHECK(remove(path) == 0);
    return 0;
}

static int test_owned_string_basics(void) {
    LorString string = LOR_STRING_INIT;
    CHECK(string == NULL);
    CHECK(strcmp(lor_string_cstr(string), "") == 0);
    CHECK(lor_string_size(string) == 0);
    CHECK(lor_string_capacity(string) == 0);
    CHECK(lor_sv_is_empty(lor_string_view(string)));

    CHECK(lor_string_append_cstr(&string, "hello") == LOR_STATUS_OK);
    CHECK(lor_string_append_char(&string, ' ') == LOR_STATUS_OK);
    CHECK(lor_string_append(&string, SV("world")) == LOR_STATUS_OK);
    CHECK(lor_string_size(string) == 11);
    CHECK(lor_string_capacity(string) >= lor_string_size(string));
    CHECK(string[lor_string_size(string)] == '\0');
    CHECK(string[5] == ' ');
    CHECK(strcmp(string, "hello world") == 0);
    CHECK(lor_string_cstr(string) == string);

    size_t capacity = lor_string_capacity(string);
    lor_string_clear(string);
    CHECK(lor_string_size(string) == 0);
    CHECK(lor_string_capacity(string) == capacity);
    CHECK(strcmp(string, "") == 0);

    CHECK(lor_string_assign_cstr(&string, "abcdef") == LOR_STATUS_OK);
    LorStringView middle = lor_sv_slice(lor_string_view(string), 2, 3);
    CHECK(lor_string_assign(&string, middle) == LOR_STATUS_OK);
    CHECK(strcmp(string, "cde") == 0);

    lor_string_deinit(&string);
    CHECK(string == NULL);
    CHECK(lor_string_size(string) == 0);
    CHECK(lor_string_capacity(string) == 0);
    return 0;
}

static int test_owned_string_binary_and_self_append(void) {
    const char binary[] = {'a', '\0', 'b'};
    LorString string = LOR_STRING_INIT;
    CHECK(lor_string_init_view(&string, lor_sv_from_parts(binary, sizeof(binary))) ==
          LOR_STATUS_OK);
    CHECK(lor_string_size(string) == sizeof(binary));
    CHECK(memcmp(string, binary, sizeof(binary)) == 0);
    CHECK(string[lor_string_size(string)] == '\0');
    lor_string_deinit(&string);

    CHECK(lor_string_init_cstr(&string, "1234567890abcdef") == LOR_STATUS_OK);
    CHECK(lor_string_size(string) == 16);
    LorStringView self = lor_string_view(string);
    CHECK(lor_string_append(&string, self) == LOR_STATUS_OK);
    CHECK(lor_string_size(string) == 32);
    CHECK(memcmp(string, "1234567890abcdef1234567890abcdef", 32) == 0);
    CHECK(string[lor_string_size(string)] == '\0');

    LorStringView suffix = lor_sv_slice(lor_string_view(string), 28, 4);
    CHECK(lor_string_append(&string, suffix) == LOR_STATUS_OK);
    CHECK(lor_sv_ends_with(lor_string_view(string), SV("cdef")));
    lor_string_deinit(&string);
    return 0;
}

static int test_owned_string_failures(void) {
    LorString string = LOR_STRING_INIT;
    CHECK(lor_string_append(NULL, SV("x")) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_string_init_cstr(NULL, "x") == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_string_init_cstr(&string, NULL) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(string == NULL);
    CHECK(lor_string_append_cstr(&string, NULL) ==
          LOR_STATUS_INVALID_ARGUMENT);

    CHECK(lor_string_append_cstr(&string, "stable") == LOR_STATUS_OK);
    LorString data = string;
    size_t size = lor_string_size(string);
    size_t capacity = lor_string_capacity(string);
    CHECK(lor_string_reserve(&string, SIZE_MAX) == LOR_STATUS_OVERFLOW);
    CHECK(string == data);
    CHECK(lor_string_size(string) == size);
    CHECK(lor_string_capacity(string) == capacity);
    CHECK(strcmp(string, "stable") == 0);

    lor_string_deinit(&string);
    lor_string_deinit(NULL);
    return 0;
}

static int test_owned_string_leakcheck(void) {
#if defined(LOR_LEAKCHECK)
    size_t before = lor_leakcheck_count();
    LorString string = LOR_STRING_INIT;
    CHECK(lor_string_append_cstr(&string, "tracked") == LOR_STATUS_OK);
    CHECK(lor_leakcheck_count() == before + 1u);
    lor_string_deinit(&string);
    CHECK(lor_leakcheck_count() == before);
#endif
    return 0;
}

static int test_owned_string_auto_cleanup(void) {
#if defined(LOR_LEAKCHECK) && \
    (defined(__GNUC__) || defined(__clang__))
    size_t before = lor_leakcheck_count();
    {
        LOR_AUTO_STRING LorString string = LOR_STRING_INIT;
        CHECK(lor_string_append_cstr(&string, "automatic") == LOR_STATUS_OK);
        CHECK(lor_leakcheck_count() == before + 1u);
    }
    CHECK(lor_leakcheck_count() == before);
#endif
    return 0;
}

int main(void) {
    CHECK(test_status_names() == 0);
    CHECK(test_view_construction_and_comparison() == 0);
    CHECK(test_view_trim_and_slice() == 0);
    CHECK(test_view_find_split_and_chop() == 0);
    CHECK(test_view_prints_exact_bytes() == 0);
    CHECK(test_owned_string_basics() == 0);
    CHECK(test_owned_string_binary_and_self_append() == 0);
    CHECK(test_owned_string_failures() == 0);
    CHECK(test_owned_string_leakcheck() == 0);
    CHECK(test_owned_string_auto_cleanup() == 0);

    puts("test_string: ok");
    return 0;
}
