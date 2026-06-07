// SPDX-License-Identifier: MIT

#include "lor/map.h"

#include <stdio.h>

typedef LOR_MAP_ENTRY(LorStringView, size_t) WordCount;

int main(void) {
    LOR_AUTO_MAP WordCount *counts = LOR_MAP_INIT;
    if (lor_map_init(counts, lor_map_config_string_view(LOR_MAP_KEY_OWNED)) !=
        LOR_STATUS_OK)
        return 1;

    LorStringView input = lor_sv_from_cstr("maps make repeated words repeated easy");
    while (!lor_sv_is_empty(input)) {
        LorStringView word;
        lor_sv_chop_char(&input, ' ', &word);
        if (lor_sv_is_empty(word)) continue;

        WordCount *entry = lor_map_find(counts, word);
        if (entry != NULL) {
            entry->value += 1u;
        } else if (lor_map_put_as(counts, WordCount, word, 1u) != LOR_STATUS_OK) {
            return 1;
        }
    }

    for (size_t i = 0; i < lor_map_size(counts); ++i) {
        lor_sv_fprint(stdout, counts[i].key);
        printf(": %zu\n", counts[i].value);
    }
    return 0;
}
