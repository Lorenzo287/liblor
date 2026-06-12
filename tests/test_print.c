// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS

#include "lor/print.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lor_test.h"

#define SV(text) ((LorStringView)LOR_SV_LITERAL(text))

typedef struct TestPoint {
    int x;
    int y;
} TestPoint;

typedef LOR_MAP_ENTRY(LorStringView, int) TestWordEntry;

static int file_equals(FILE *file, const void *expected, size_t expected_size) {
    unsigned char buffer[256];

    if (fflush(file) != 0 || fseek(file, 0, SEEK_SET) != 0) return 0;
    size_t size = fread(buffer, 1, sizeof(buffer), file);
    return size == expected_size && memcmp(buffer, expected, expected_size) == 0;
}

static LorStatus print_point(FILE *out, const void *value) {
    const TestPoint *point = value;
    return fprintf(out, "Point(%d,%d)", point->x, point->y) < 0
               ? LOR_STATUS_SYSTEM_ERROR
               : LOR_STATUS_OK;
}

static int test_generic_values(void) {
    FILE *file = tmpfile();
    CHECK(file != NULL);

    CHECK(lor_fprint(file, 10, "hello", 3.5, (bool)true, (char)'A') ==
          LOR_STATUS_OK);
    static const char expected[] = "10 hello 3.5 true A\n";
    CHECK(file_equals(file, expected, sizeof(expected) - 1u));
    CHECK(fclose(file) == 0);
    return 0;
}

static int test_end_markers(void) {
    FILE *file = tmpfile();
    CHECK(file != NULL);

    CHECK(lor_fprint(file, lor_end("?"), "hello", lor_end("!"), "world") ==
          LOR_STATUS_OK);
    static const char first_expected[] = "hello world!";
    CHECK(file_equals(file, first_expected, sizeof(first_expected) - 1u));
    CHECK(fclose(file) == 0);

    file = tmpfile();
    CHECK(file != NULL);
    CHECK(lor_fprint(file, "no newline", lor_end("")) == LOR_STATUS_OK);
    static const char second_expected[] = "no newline";
    CHECK(file_equals(file, second_expected, sizeof(second_expected) - 1u));
    CHECK(fclose(file) == 0);
    return 0;
}

static int test_views_null_strings_and_config(void) {
    const char binary[] = {'a', '\0', 'b'};
    FILE *file = tmpfile();
    CHECK(file != NULL);

    LorPrintConfig config = {SV("|"), SV(".")};
    CHECK(lor_fprint_with(file, config, lor_sv_from_parts(binary, sizeof(binary)),
                          (const char *)NULL) == LOR_STATUS_OK);
    const char expected[] = {'a', '\0', 'b', '|', '(', 'n', 'u', 'l', 'l', ')', '.'};
    CHECK(file_equals(file, expected, sizeof(expected)));
    CHECK(fclose(file) == 0);

    LorString string = LOR_STRING_INIT;
    CHECK(lor_string_assign_cstr(&string, "owned") == LOR_STATUS_OK);
    file = tmpfile();
    CHECK(file != NULL);
    CHECK(lor_fprint(file, string, lor_string_view(string)) == LOR_STATUS_OK);
    static const char string_expected[] = "owned owned\n";
    CHECK(file_equals(file, string_expected, sizeof(string_expected) - 1u));
    CHECK(fclose(file) == 0);
    lor_string_deinit(&string);
    return 0;
}

static int test_custom_and_explicit_values(void) {
    TestPoint point = {2, 7};
    FILE *file = tmpfile();
    CHECK(file != NULL);

    CHECK(lor_fprint(file, "point", lor_print_custom(&point, print_point)) ==
          LOR_STATUS_OK);
    static const char expected[] = "point Point(2,7)\n";
    CHECK(file_equals(file, expected, sizeof(expected) - 1u));
    CHECK(fclose(file) == 0);

    file = tmpfile();
    CHECK(file != NULL);
    LorPrintConfig config = LOR_PRINT_CONFIG_INIT;
    CHECK(lor_fprint_values(file, config, NULL, 0) == LOR_STATUS_OK);
    CHECK(file_equals(file, "\n", 1));
    CHECK(fclose(file) == 0);
    return 0;
}

static int test_liblor_values(void) {
    FILE *file = tmpfile();
    CHECK(file != NULL);

    LorArena arena = LOR_ARENA_INIT;
    LorMmap map = {0};
    LorRandom random = LOR_RANDOM_INIT;
    CHECK(lor_fprint(file, arena, map, random) == LOR_STATUS_OK);

    CHECK(fflush(file) == 0);
    CHECK(fseek(file, 0, SEEK_SET) == 0);
    char buffer[256];
    size_t size = fread(buffer, 1, sizeof(buffer) - 1u, file);
    buffer[size] = '\0';
    CHECK(strstr(buffer,
                 "LorArena(backend=heap, used=0, capacity=0, committed=0)") != NULL);
    CHECK(strstr(buffer, "LorMmap(data=") != NULL);
    CHECK(strstr(buffer, ", size=0)") != NULL);
    CHECK(strstr(buffer, "LorRandom(state=6364136223846793006, increment=1)") !=
          NULL);
    CHECK(fclose(file) == 0);
    return 0;
}

static int test_liblor_containers(void) {
    int *numbers = LOR_ARRAY_INIT;
    CHECK(lor_array_push_as(numbers, int, 1) == LOR_STATUS_OK);
    CHECK(lor_array_push_as(numbers, int, 2) == LOR_STATUS_OK);
    CHECK(lor_array_push_as(numbers, int, 3) == LOR_STATUS_OK);

    LorStringView *views = LOR_ARRAY_INIT;
    CHECK(lor_array_push_as(views, LorStringView, .data = "alpha", .size = 5) ==
          LOR_STATUS_OK);
    const char escaped[] = {'l', 'i', 'n', 'e', '\n'};
    LorStringView escaped_view = lor_sv_from_parts(escaped, sizeof(escaped));
    CHECK(lor_array_push(views, escaped_view) == LOR_STATUS_OK);

    int *set = LOR_SET_INIT;
    CHECK(lor_set_add_as(set, int, 4) == LOR_STATUS_OK);
    CHECK(lor_set_add_as(set, int, 7) == LOR_STATUS_OK);
    int *empty_set = LOR_SET_INIT;

    TestWordEntry *map = LOR_MAP_INIT;
    LorStringView one = LOR_SV_LITERAL("one");
    LorStringView two = LOR_SV_LITERAL("two");
    CHECK(lor_map_put_as(map, TestWordEntry, one, 1) == LOR_STATUS_OK);
    CHECK(lor_map_put_as(map, TestWordEntry, two, 2) == LOR_STATUS_OK);

    TestPoint *points = LOR_ARRAY_INIT;
    CHECK(lor_array_push_as(points, TestPoint, .x = 2, .y = 7) == LOR_STATUS_OK);

    FILE *file = tmpfile();
    CHECK(file != NULL);
    CHECK(lor_fprint(file, lor_print_array(numbers), lor_print_array(views),
                     lor_print_set(set), lor_print_set(empty_set),
                     lor_print_map(map),
                     lor_print_array_with(points, print_point)) == LOR_STATUS_OK);
    static const char expected[] =
        "[1, 2, 3] [\"alpha\", \"line\\n\"] {4, 7} set() "
        "{\"one\": 1, \"two\": 2} [Point(2,7)]\n";
    CHECK(file_equals(file, expected, sizeof(expected) - 1u));
    CHECK(fclose(file) == 0);

    lor_array_deinit(&points);
    lor_map_deinit(&map);
    lor_set_deinit(&empty_set);
    lor_set_deinit(&set);
    lor_array_deinit(&views);
    lor_array_deinit(&numbers);
    return 0;
}

static int test_maximum_arguments_and_single_evaluation(void) {
    FILE *file = tmpfile();
    CHECK(file != NULL);

    int a = 0;
    int b = 0;
    CHECK(lor_fprint(file, ++a, ++b, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                     lor_end("")) == LOR_STATUS_OK);
    CHECK(a == 1);
    CHECK(b == 1);
    static const char expected[] = "1 1 3 4 5 6 7 8 9 10 11 12 13 14 15";
    CHECK(file_equals(file, expected, sizeof(expected) - 1u));
    CHECK(fclose(file) == 0);
    return 0;
}

static int test_invalid_explicit_input(void) {
    FILE *file = tmpfile();
    CHECK(file != NULL);
    LorPrintConfig config = LOR_PRINT_CONFIG_INIT;

    CHECK(lor_fprint_values(NULL, config, NULL, 0) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_fprint_values(file, config, NULL, 1) == LOR_STATUS_INVALID_ARGUMENT);

    LorPrintValue invalid = lor_print_value_init((LorPrintKind)99);
    CHECK(lor_fprint_values(file, config, &invalid, 1) ==
          LOR_STATUS_INVALID_ARGUMENT);

    LorPrintValue custom = lor_print_custom(NULL, NULL);
    CHECK(lor_fprint_values(file, config, &custom, 1) ==
          LOR_STATUS_INVALID_ARGUMENT);

    LorPrintConfig invalid_config = {{NULL, 1}, SV("\n")};
    CHECK(lor_fprint_values(file, invalid_config, NULL, 0) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(fclose(file) == 0);
    return 0;
}

int main(void) {
    CHECK(LOR_HAS_GENERIC_PRINT);
    CHECK(test_generic_values() == 0);
    CHECK(test_end_markers() == 0);
    CHECK(test_views_null_strings_and_config() == 0);
    CHECK(test_custom_and_explicit_values() == 0);
    CHECK(test_liblor_values() == 0);
    CHECK(test_liblor_containers() == 0);
    CHECK(test_maximum_arguments_and_single_evaluation() == 0);
    CHECK(test_invalid_explicit_input() == 0);

    puts("test_print: ok");
    return 0;
}
