// SPDX-License-Identifier: MIT

#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <cassert>

#if HAS_COMPOUND_LITERALS
#error "C compound literals must not be exposed to C++"
#endif

#ifdef array_push_as
#error "C-only short aliases must not be exposed to C++"
#endif

#ifdef array_push_auto
#error "Unavailable automatic short aliases must not be exposed to C++"
#endif

#ifdef print
#error "C11 generic printing must not be exposed to C++"
#endif

int main() {
    int *numbers = ARRAY_INIT;
    int value = 9;
    assert(array_push(numbers, value) == STATUS_OK);
    assert(array_size(numbers) == 1u);
    array_deinit(&numbers);
    return 0;
}
