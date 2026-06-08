// SPDX-License-Identifier: MIT

#include "lor/type.h"

#include <stdio.h>
#include <string.h>

#include "lor_test.h"

typedef struct TestPoint {
    int x;
    int y;
} TestPoint;

static int test_type_kinds_and_names(void) {
    const char *text = "liblor";
    LorString string = LOR_STRING_INIT;
    void *pointer = NULL;
    int *typed_pointer = NULL;
    TestPoint point = {1, 2};
    LorStringView view = LOR_SV_LITERAL("view");
    LorArena arena = LOR_ARENA_INIT;
    LorArenaConfig arena_config = {0};
    LorArenaMark mark = LOR_ARENA_MARK_INIT;
    LorScratch scratch = LOR_SCRATCH_INIT;
    LorMmap map = {0};
    LorLeakStats stats = {0};
    LorRandom random = LOR_RANDOM_INIT;

    CHECK(lor_type_kind((_Bool)1) == LOR_TYPE_BOOL);
    CHECK(lor_type_kind((char)'a') == LOR_TYPE_CHAR);
    CHECK(lor_type_kind((signed char)-1) == LOR_TYPE_SIGNED_CHAR);
    CHECK(lor_type_kind((unsigned char)1) == LOR_TYPE_UNSIGNED_CHAR);
    CHECK(lor_type_kind((short)-1) == LOR_TYPE_SHORT);
    CHECK(lor_type_kind((unsigned short)1) == LOR_TYPE_UNSIGNED_SHORT);
    CHECK(lor_type_kind(-1) == LOR_TYPE_INT);
    CHECK(lor_type_kind(1u) == LOR_TYPE_UNSIGNED_INT);
    CHECK(lor_type_kind(1l) == LOR_TYPE_LONG);
    CHECK(lor_type_kind(1ul) == LOR_TYPE_UNSIGNED_LONG);
    CHECK(lor_type_kind(1ll) == LOR_TYPE_LONG_LONG);
    CHECK(lor_type_kind(1ull) == LOR_TYPE_UNSIGNED_LONG_LONG);
    CHECK(lor_type_kind(1.0f) == LOR_TYPE_FLOAT);
    CHECK(lor_type_kind(1.0) == LOR_TYPE_DOUBLE);
    CHECK(lor_type_kind(1.0l) == LOR_TYPE_LONG_DOUBLE);
    CHECK(lor_type_kind("text") == LOR_TYPE_CSTRING);
    CHECK(lor_type_kind(text) == LOR_TYPE_CSTRING);
    CHECK(lor_type_kind(string) == LOR_TYPE_CSTRING);
    CHECK(lor_type_kind(pointer) == LOR_TYPE_POINTER);
    CHECK(lor_type_kind(typed_pointer) == LOR_TYPE_OTHER);
    CHECK(lor_type_kind(point) == LOR_TYPE_OTHER);
    CHECK(lor_type_kind(view) == LOR_TYPE_STRING_VIEW);
    CHECK(lor_type_kind(arena_config) == LOR_TYPE_ARENA_CONFIG);
    CHECK(lor_type_kind(arena) == LOR_TYPE_ARENA);
    CHECK(lor_type_kind(mark) == LOR_TYPE_ARENA_MARK);
    CHECK(lor_type_kind(scratch) == LOR_TYPE_SCRATCH);
    CHECK(lor_type_kind(map) == LOR_TYPE_MMAP);
    CHECK(lor_type_kind(stats) == LOR_TYPE_LEAK_STATS);
    CHECK(lor_type_kind(random) == LOR_TYPE_RANDOM);

    CHECK(strcmp(lor_type_name(1u), "unsigned int") == 0);
    CHECK(strcmp(lor_type_kind_name(LOR_TYPE_CSTRING), "C string") == 0);
    CHECK(strcmp(lor_type_name(view), "LorStringView") == 0);
    CHECK(strcmp(lor_type_name(arena), "LorArena") == 0);
    CHECK(strcmp(lor_type_name(random), "LorRandom") == 0);
    CHECK(strcmp(lor_type_kind_name((LorTypeKind)99), "other") == 0);
    return 0;
}

static int test_type_expressions_are_not_evaluated(void) {
    int value = 4;
    CHECK(lor_type_kind(value++) == LOR_TYPE_INT);
    CHECK(value == 4);
    CHECK(strcmp(lor_type_name(++value), "int") == 0);
    CHECK(value == 4);
    return 0;
}

static int test_typeof_declarations(void) {
#if LOR_HAS_TYPEOF
    TestPoint point = {3, 5};
    lor_typeof(point) copy = point;
    lor_typeof(point.x) coordinate = 8;
    CHECK(copy.x == 3);
    CHECK(copy.y == 5);
    CHECK(coordinate == 8);
#endif
    return 0;
}

int main(void) {
    CHECK(LOR_HAS_GENERIC_SELECTION);
    CHECK(test_type_kinds_and_names() == 0);
    CHECK(test_type_expressions_are_not_evaluated() == 0);
    CHECK(test_typeof_declarations() == 0);

    puts("test_type: ok");
    return 0;
}
