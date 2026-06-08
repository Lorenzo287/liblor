// SPDX-License-Identifier: MIT

#include "lor/array.h"

#include "lor/memory.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lor_test.h"

typedef struct TestPoint {
    int x;
    int y;
} TestPoint;

static int test_array_basics(void) {
    int *numbers = LOR_ARRAY_INIT;
    CHECK(lor_array_size(numbers) == 0);
    CHECK(lor_array_capacity(numbers) == 0);
    CHECK(lor_array_element_size(numbers) == 0);

    int first = 10;
    CHECK(lor_array_push(numbers, first) == LOR_STATUS_OK);
    CHECK(lor_array_push_as(numbers, int, 20) == LOR_STATUS_OK);
    CHECK(lor_array_push_as(numbers, int, 30) == LOR_STATUS_OK);
    CHECK(lor_array_size(numbers) == 3);
    CHECK(lor_array_capacity(numbers) >= 3);
    CHECK(lor_array_element_size(numbers) == sizeof(*numbers));
    CHECK(numbers[0] == 10);
    CHECK(numbers[1] == 20);
    CHECK(numbers[2] == 30);

    size_t capacity = lor_array_capacity(numbers);
    CHECK(lor_array_reserve(numbers, capacity + 5u) == LOR_STATUS_OK);
    CHECK(lor_array_capacity(numbers) >= capacity + 5u);
    CHECK(lor_array_size(numbers) == 3);

    lor_array_deinit(&numbers);
    CHECK(numbers == NULL);
    return 0;
}

static int test_array_push_forms(void) {
    int *numbers = LOR_ARRAY_INIT;
    int value = 10;
    CHECK(lor_array_push(numbers, value) == LOR_STATUS_OK);
    CHECK(lor_array_push_as(numbers, int, 20) == LOR_STATUS_OK);

#if LOR_HAS_ARRAY_PUSH_AUTO
    int next = 30;
    CHECK(lor_array_push_auto(numbers, next++) == LOR_STATUS_OK);
    CHECK(next == 31);
    CHECK(lor_array_push_auto(numbers, (short)40) == LOR_STATUS_OK);
    CHECK(lor_array_size(numbers) == 4);
    CHECK(numbers[2] == 30);
    CHECK(numbers[3] == 40);

    TestPoint *points = LOR_ARRAY_INIT;
    TestPoint point = {.x = 1, .y = 2};
    CHECK(lor_array_push_auto(points, point) == LOR_STATUS_OK);
    CHECK(lor_array_push_auto(
              points, ((TestPoint){.x = 3, .y = 4})) ==
          LOR_STATUS_OK);
    CHECK(points[0].x == 1 && points[0].y == 2);
    CHECK(points[1].x == 3 && points[1].y == 4);
    lor_array_deinit(&points);
#endif

    lor_array_deinit(&numbers);
    return 0;
}

static int test_array_append_resize_and_aliases(void) {
    int *numbers = LOR_ARRAY_INIT;
    const int initial[] = {1, 2, 3, 4, 5, 6, 7, 8};
    CHECK(lor_array_append(numbers, initial, 8) == LOR_STATUS_OK);

    CHECK(lor_array_append(numbers, numbers, lor_array_size(numbers)) ==
          LOR_STATUS_OK);
    CHECK(lor_array_size(numbers) == 16);
    CHECK(memcmp(numbers, initial, sizeof(initial)) == 0);
    CHECK(memcmp(numbers + 8, initial, sizeof(initial)) == 0);

    CHECK(lor_array_append(numbers, numbers + 12, 4) == LOR_STATUS_OK);
    CHECK(lor_array_size(numbers) == 20);
    CHECK(memcmp(numbers + 16, initial + 4, 4 * sizeof(*numbers)) == 0);

    CHECK(lor_array_resize(numbers, 24) == LOR_STATUS_OK);
    CHECK(lor_array_size(numbers) == 24);
    for (size_t i = 20; i < 24; ++i) CHECK(numbers[i] == 0);

    size_t capacity = lor_array_capacity(numbers);
    CHECK(lor_array_resize(numbers, 3) == LOR_STATUS_OK);
    CHECK(lor_array_size(numbers) == 3);
    CHECK(lor_array_capacity(numbers) == capacity);

    lor_array_clear(numbers);
    CHECK(lor_array_size(numbers) == 0);
    CHECK(lor_array_capacity(numbers) == capacity);
    lor_array_deinit(&numbers);
    return 0;
}

static int test_array_insert_ranges_and_capacity(void) {
    int *numbers = LOR_ARRAY_INIT;
    const int initial[] = {1, 2, 5, 6};
    CHECK(lor_array_append(numbers, initial, 4) == LOR_STATUS_OK);

    CHECK(lor_array_insert_as(numbers, 2, int, 3) == LOR_STATUS_OK);
    int four = 4;
    CHECK(lor_array_insert(numbers, 3, four) == LOR_STATUS_OK);

    const int prefix[] = {-1, 0};
    CHECK(lor_array_insert_many(numbers, 0, prefix, 2) == LOR_STATUS_OK);
    const int ordered[] = {-1, 0, 1, 2, 3, 4, 5, 6};
    CHECK(lor_array_size(numbers) == 8);
    CHECK(memcmp(numbers, ordered, sizeof(ordered)) == 0);

    // The source spans the insertion point, so insertion must preserve a copy.
    CHECK(lor_array_insert_many(numbers, 4, numbers + 2, 4) ==
          LOR_STATUS_OK);
    const int inserted[] = {-1, 0, 1, 2, 1, 2, 3, 4, 3, 4, 5, 6};
    CHECK(lor_array_size(numbers) == 12);
    CHECK(memcmp(numbers, inserted, sizeof(inserted)) == 0);
    CHECK(lor_array_last(numbers) != NULL);
    CHECK(*lor_array_last(numbers) == 6);

    size_t size = lor_array_size(numbers);
    CHECK(lor_array_insert_as(numbers, size + 1u, int, 99) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_array_size(numbers) == size);

    CHECK(lor_array_remove_range(numbers, 4, 4));
    CHECK(lor_array_size(numbers) == 8);
    CHECK(memcmp(numbers, ordered, sizeof(ordered)) == 0);
    CHECK(lor_array_remove_range(numbers, lor_array_size(numbers), 0));
    CHECK(!lor_array_remove_range(numbers, lor_array_size(numbers) + 1u, 0));
    CHECK(!lor_array_remove_range(numbers, 7, 2));

    int *copy = LOR_ARRAY_INIT;
    CHECK(lor_array_append_array(copy, numbers) == LOR_STATUS_OK);
    CHECK(memcmp(copy, ordered, sizeof(ordered)) == 0);
    CHECK(lor_array_append_array(copy, copy) == LOR_STATUS_OK);
    CHECK(lor_array_size(copy) == 16);

    short *shorts = LOR_ARRAY_INIT;
    CHECK(lor_array_push_as(shorts, short, 7) == LOR_STATUS_OK);
    CHECK(lor_array_append_array(copy, shorts) ==
          LOR_STATUS_INVALID_ARGUMENT);

    CHECK(lor_array_reserve(copy, 64) == LOR_STATUS_OK);
    CHECK(lor_array_capacity(copy) >= 64);
    CHECK(lor_array_shrink_to_fit(copy) == LOR_STATUS_OK);
    CHECK(lor_array_capacity(copy) == lor_array_size(copy));

    lor_array_clear(copy);
    CHECK(lor_array_shrink_to_fit(copy) == LOR_STATUS_OK);
    CHECK(copy == NULL);
    CHECK(lor_array_last(copy) == NULL);

    lor_array_deinit(&shorts);
    lor_array_deinit(&numbers);
    return 0;
}

static int test_array_structs_and_removal(void) {
    TestPoint *points = LOR_ARRAY_INIT;
    CHECK(lor_array_push_as(points, TestPoint, .x = 1, .y = 2) ==
          LOR_STATUS_OK);
    CHECK(lor_array_push_as(points, TestPoint, .x = 3, .y = 4) ==
          LOR_STATUS_OK);
    CHECK(lor_array_push_as(points, TestPoint, .x = 5, .y = 6) ==
          LOR_STATUS_OK);

    TestPoint removed;
    CHECK(lor_array_remove(points, 1, &removed));
    CHECK(removed.x == 3 && removed.y == 4);
    CHECK(lor_array_size(points) == 2);
    CHECK(points[1].x == 5 && points[1].y == 6);

    CHECK(lor_array_push_as(points, TestPoint, .x = 7, .y = 8) ==
          LOR_STATUS_OK);
    CHECK(lor_array_remove_unordered(points, 0, &removed));
    CHECK(removed.x == 1 && removed.y == 2);
    CHECK(lor_array_size(points) == 2);
    CHECK(points[0].x == 7 && points[0].y == 8);

    CHECK(lor_array_pop(points, &removed));
    CHECK(removed.x == 5 && removed.y == 6);
    CHECK(lor_array_size(points) == 1);
    CHECK(lor_array_pop(points, NULL));
    CHECK(!lor_array_pop(points, NULL));
    CHECK(!lor_array_remove(points, 0, NULL));
    CHECK(!lor_array_remove_unordered(points, 0, NULL));

    lor_array_deinit(&points);
    return 0;
}

static int test_array_alignment_and_failures(void) {
    max_align_t *aligned = LOR_ARRAY_INIT;
    CHECK(lor_array_resize(aligned, 1) == LOR_STATUS_OK);
    CHECK((uintptr_t)aligned % _Alignof(max_align_t) == 0);
    lor_array_deinit(&aligned);

    int *numbers = LOR_ARRAY_INIT;
    CHECK(lor_array_reserve_raw(NULL, sizeof(*numbers), 1) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_array_reserve_raw(&numbers, 0, 1) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_array_append_raw(&numbers, sizeof(*numbers), NULL, 1) ==
          LOR_STATUS_INVALID_ARGUMENT);

    CHECK(lor_array_push_as(numbers, int, 7) == LOR_STATUS_OK);
    int *data = numbers;
    size_t size = lor_array_size(numbers);
    size_t capacity = lor_array_capacity(numbers);
    CHECK(lor_array_reserve_raw(&numbers, sizeof(short), capacity + 1u) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_array_reserve(numbers, SIZE_MAX) == LOR_STATUS_OVERFLOW);
    CHECK(lor_array_append_raw(&numbers, sizeof(*numbers), numbers, SIZE_MAX) ==
          LOR_STATUS_OVERFLOW);
    CHECK(lor_array_append_raw(&numbers, sizeof(*numbers), numbers, 2) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(numbers == data);
    CHECK(lor_array_size(numbers) == size);
    CHECK(lor_array_capacity(numbers) == capacity);
    CHECK(numbers[0] == 7);

    lor_array_deinit(&numbers);
    lor_array_deinit(NULL);
    return 0;
}

static int test_array_cleanup_and_leakcheck(void) {
#if defined(LOR_LEAKCHECK)
    size_t before = lor_leakcheck_count();
    {
        LOR_AUTO_ARRAY int *numbers = LOR_ARRAY_INIT;
        CHECK(lor_array_push_as(numbers, int, 42) == LOR_STATUS_OK);
        CHECK(lor_leakcheck_count() == before + 1u);
    }
    CHECK(lor_leakcheck_count() == before);
#endif
    return 0;
}

int main(void) {
    CHECK(test_array_basics() == 0);
    CHECK(test_array_push_forms() == 0);
    CHECK(test_array_append_resize_and_aliases() == 0);
    CHECK(test_array_insert_ranges_and_capacity() == 0);
    CHECK(test_array_structs_and_removal() == 0);
    CHECK(test_array_alignment_and_failures() == 0);
    CHECK(test_array_cleanup_and_leakcheck() == 0);

    puts("test_array: ok");
    return 0;
}
