// SPDX-License-Identifier: MIT

#include "lor/array.h"
#include "lor/cli.h"
#include "lor/concurrency.h"
#include "lor/map.h"
#include "lor/memory.h"
#include "lor/numeric.h"
#include "lor/print.h"
#include "lor/random.h"
#include "lor/set.h"
#include "lor/status.h"
#include "lor/string.h"
#include "lor/type.h"

#include <cassert>

#if LOR_HAS_COMPOUND_LITERALS
#error "C compound literals must not be exposed to C++"
#endif

#ifdef lor_array_push_as
#error "lor_array_push_as must not be exposed to C++"
#endif

#ifdef lor_map_put_as
#error "lor_map_put_as must not be exposed to C++"
#endif

#ifdef lor_set_add_as
#error "lor_set_add_as must not be exposed to C++"
#endif

struct IntEntry {
    int key;
    int value;
};

int main() {
    int *numbers = LOR_ARRAY_INIT;
    int number = 42;
    assert(lor_array_push(numbers, number) == LOR_STATUS_OK);
    assert(lor_array_size(numbers) == 1u);
    assert(numbers[0] == number);

    IntEntry *map = nullptr;
    assert(lor_map_init(map, lor_map_config_bytes()) == LOR_STATUS_OK);
    IntEntry entry = {7, 70};
    assert(lor_map_set(map, entry) == LOR_STATUS_OK);
    int key = 7;
    const IntEntry *found =
        static_cast<const IntEntry *>(lor_map_find_const(map, key));
    assert(found != nullptr);
    assert(found->value == 70);

    int *set = nullptr;
    assert(lor_set_init(set, lor_set_config_bytes()) == LOR_STATUS_OK);
    assert(lor_set_add(set, number) == LOR_STATUS_OK);
    assert(lor_set_contains(set, number));

    LorString string = LOR_STRING_INIT;
    assert(lor_string_append_cstr(&string, "cpp") == LOR_STATUS_OK);
    assert(lor_string_size(string) == 3u);

    lor_string_deinit(&string);
    lor_set_deinit(&set);
    lor_map_deinit(&map);
    lor_array_deinit(&numbers);
    return 0;
}
