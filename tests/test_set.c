// SPDX-License-Identifier: MIT

#include "lor/set.h"

#include "lor/memory.h"

#include <stdio.h>

#include "lor_test.h"

static uint64_t test_set_hash(const void *key, size_t key_size,
                              void *context) {
    (void)key;
    (void)key_size;
    (void)context;
    return 0;
}

static int test_set_equal(const void *a, const void *b, size_t key_size,
                          void *context) {
    (void)context;
    return key_size == sizeof(int) &&
           *(const int *)a == *(const int *)b;
}

static int set_has_all(const int *set, const int *values, size_t count) {
    if (lor_set_size(set) != count) return 0;
    for (size_t i = 0; i < count; ++i) {
        if (!lor_set_contains(set, values[i])) return 0;
    }
    return 1;
}

static int test_set_basics(void) {
    int *set = LOR_SET_INIT;
    CHECK(lor_set_size(set) == 0);
    CHECK(lor_set_capacity(set) == 0);

    int one = 1;
    CHECK(lor_set_add(set, one) == LOR_STATUS_OK);
    CHECK(lor_set_add_as(set, int, 2) == LOR_STATUS_OK);
#if LOR_HAS_SET_AUTO
    int next = 3;
    CHECK(lor_set_add_auto(set, next++) == LOR_STATUS_OK);
    CHECK(next == 4);
#else
    CHECK(lor_set_add_as(set, int, 3) == LOR_STATUS_OK);
#endif
    CHECK(lor_set_add_as(set, int, 2) == LOR_STATUS_OK);
    CHECK(lor_set_size(set) == 3);

    int sum = 0;
    for (size_t i = 0; i < lor_set_size(set); ++i) sum += set[i];
    CHECK(sum == 6);

    int two = 2;
    CHECK(lor_set_contains(set, two));
    CHECK(lor_set_contains_as(set, int, 3));
    CHECK(!lor_set_contains_as(set, int, 9));

    CHECK(lor_set_reserve(set, 50) == LOR_STATUS_OK);
    CHECK(lor_set_capacity(set) >= 50);
    size_t capacity = lor_set_capacity(set);

    CHECK(lor_set_remove(set, two));
    CHECK(!lor_set_remove(set, two));
    CHECK(lor_set_size(set) == 2);

    lor_set_clear(set);
    CHECK(lor_set_size(set) == 0);
    CHECK(lor_set_capacity(set) == capacity);

    lor_set_deinit(&set);
    CHECK(set == NULL);
    lor_set_deinit(&set);
    lor_set_deinit(NULL);
    return 0;
}

static int test_set_algebra(void) {
    int *a = LOR_SET_INIT;
    int *b = LOR_SET_INIT;
    for (int i = 1; i <= 3; ++i)
        CHECK(lor_set_add(a, i) == LOR_STATUS_OK);
    for (int i = 3; i <= 4; ++i)
        CHECK(lor_set_add(b, i) == LOR_STATUS_OK);

    int *result = LOR_SET_INIT;
    CHECK(lor_set_union(result, a, b) == LOR_STATUS_OK);
    const int union_values[] = {1, 2, 3, 4};
    CHECK(set_has_all(result, union_values, 4));
    lor_set_deinit(&result);

    CHECK(lor_set_intersection(result, a, b) == LOR_STATUS_OK);
    const int intersection_values[] = {3};
    CHECK(set_has_all(result, intersection_values, 1));
    lor_set_deinit(&result);

    CHECK(lor_set_difference(result, a, b) == LOR_STATUS_OK);
    const int difference_values[] = {1, 2};
    CHECK(set_has_all(result, difference_values, 2));
    lor_set_deinit(&result);

    CHECK(lor_set_symmetric_difference(result, a, b) == LOR_STATUS_OK);
    const int symmetric_values[] = {1, 2, 4};
    CHECK(set_has_all(result, symmetric_values, 3));
    lor_set_deinit(&result);

    CHECK(lor_set_union(result, a, a) == LOR_STATUS_OK);
    CHECK(lor_set_equal(result, a));
    lor_set_deinit(&result);

    CHECK(lor_set_is_subset(a, a));
    CHECK(lor_set_is_superset(a, a));
    CHECK(!lor_set_is_proper_subset(a, a));
    CHECK(!lor_set_is_proper_superset(a, a));
    CHECK(!lor_set_is_disjoint(a, b));

    int *small = LOR_SET_INIT;
    CHECK(lor_set_add_as(small, int, 1) == LOR_STATUS_OK);
    CHECK(lor_set_is_subset(small, a));
    CHECK(lor_set_is_proper_subset(small, a));
    CHECK(lor_set_is_superset(a, small));
    CHECK(lor_set_is_proper_superset(a, small));
    CHECK(!lor_set_equal(small, a));

    int *disjoint = LOR_SET_INIT;
    CHECK(lor_set_add_as(disjoint, int, 9) == LOR_STATUS_OK);
    CHECK(lor_set_is_disjoint(a, disjoint));

    int *empty = LOR_SET_INIT;
    CHECK(lor_set_is_subset(empty, a));
    CHECK(lor_set_is_proper_subset(empty, a));
    CHECK(lor_set_is_disjoint(empty, a));
    CHECK(lor_set_equal(empty, NULL));

    CHECK(lor_set_add_as(result, int, 99) == LOR_STATUS_OK);
    CHECK(lor_set_union(result, a, b) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_set_contains_as(result, int, 99));

    lor_set_deinit(&empty);
    lor_set_deinit(&disjoint);
    lor_set_deinit(&small);
    lor_set_deinit(&result);
    lor_set_deinit(&b);
    lor_set_deinit(&a);
    return 0;
}

static int test_set_string_ownership(void) {
    LorStringView *a = LOR_SET_INIT;
    LorStringView *b = LOR_SET_INIT;
    CHECK(lor_set_init(
              a, lor_set_config_string_view(LOR_SET_KEY_OWNED)) ==
          LOR_STATUS_OK);
    CHECK(lor_set_init(
              b, lor_set_config_string_view(LOR_SET_KEY_OWNED)) ==
          LOR_STATUS_OK);

    char mutable[] = "alpha";
    LorStringView alpha = lor_sv_from_cstr(mutable);
    LorStringView beta = lor_sv_from_cstr("beta");
    LorStringView gamma = lor_sv_from_cstr("gamma");
    CHECK(lor_set_add(a, alpha) == LOR_STATUS_OK);
    CHECK(lor_set_add(a, beta) == LOR_STATUS_OK);
    CHECK(lor_set_add(b, beta) == LOR_STATUS_OK);
    CHECK(lor_set_add(b, gamma) == LOR_STATUS_OK);
    mutable[0] = 'x';

    LorStringView *combined = LOR_SET_INIT;
    CHECK(lor_set_union(combined, a, b) == LOR_STATUS_OK);
    lor_set_deinit(&a);
    lor_set_deinit(&b);

    LorStringView lookup = lor_sv_from_cstr("alpha");
    CHECK(lor_set_contains(combined, lookup));
    CHECK(lor_set_contains(combined, beta));
    CHECK(lor_set_contains(combined, gamma));

    LorStringView *borrowed = LOR_SET_INIT;
    CHECK(lor_set_init(
              borrowed,
              lor_set_config_string_view(LOR_SET_KEY_BORROWED)) ==
          LOR_STATUS_OK);
    LorStringView borrowed_key = lor_sv_from_cstr("borrowed");
    CHECK(lor_set_add(borrowed, borrowed_key) == LOR_STATUS_OK);

    LorStringView *mixed = LOR_SET_INIT;
    CHECK(lor_set_union(mixed, combined, borrowed) == LOR_STATUS_OK);
    lor_set_deinit(&borrowed);
    lor_set_deinit(&combined);
    CHECK(lor_set_contains(mixed, lookup));
    CHECK(lor_set_contains(mixed, borrowed_key));

    lor_set_deinit(&mixed);
    return 0;
}

static int test_set_failures_and_cleanup(void) {
    int *set = LOR_SET_INIT;
    LorSetConfig invalid = {.hash = test_set_hash};
    CHECK(lor_set_init(set, invalid) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(set == NULL);
    CHECK(lor_set_reserve(set, SIZE_MAX) == LOR_STATUS_OVERFLOW);
    CHECK(set == NULL);
    CHECK(lor_set_union_raw(NULL, sizeof(int), NULL, NULL) ==
          LOR_STATUS_INVALID_ARGUMENT);

    int *bytes = LOR_SET_INIT;
    int *custom = LOR_SET_INIT;
    CHECK(lor_set_add_as(bytes, int, 1) == LOR_STATUS_OK);
    LorSetConfig custom_config = {
        .hash = test_set_hash,
        .equal = test_set_equal,
    };
    CHECK(lor_set_init(custom, custom_config) == LOR_STATUS_OK);
    CHECK(lor_set_add_as(custom, int, 1) == LOR_STATUS_OK);
    int *result = LOR_SET_INIT;
    CHECK(lor_set_union(result, bytes, custom) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(result == NULL);
    CHECK(!lor_set_equal(bytes, custom));
    lor_set_deinit(&custom);
    lor_set_deinit(&bytes);

#if defined(LOR_LEAKCHECK)
    size_t before = lor_leakcheck_count();
    {
        LOR_AUTO_SET LorStringView *words = LOR_SET_INIT;
        CHECK(lor_set_init(
                  words,
                  lor_set_config_string_view(LOR_SET_KEY_OWNED)) ==
              LOR_STATUS_OK);
        LorStringView tracked = lor_sv_from_cstr("tracked");
        CHECK(lor_set_add(words, tracked) == LOR_STATUS_OK);
        CHECK(lor_leakcheck_count() > before);
    }
    CHECK(lor_leakcheck_count() == before);
#endif
    return 0;
}

int main(void) {
    CHECK(test_set_basics() == 0);
    CHECK(test_set_algebra() == 0);
    CHECK(test_set_string_ownership() == 0);
    CHECK(test_set_failures_and_cleanup() == 0);

    puts("test_set: ok");
    return 0;
}
