// SPDX-License-Identifier: MIT

#include "lor/string.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    const char source[] = "alpha,beta,gamma";

    // LorStringView is public data, so it can be initialized directly.
    LorStringView input = {
        .data = source,
        .size = sizeof(source) - 1u,
    };

    size_t comma = 0;
    while (comma < input.size && input.data[comma] != ',') comma += 1u;

    // Views can also be sliced directly with pointer arithmetic.
    LorStringView first = {
        .data = input.data,
        .size = comma,
    };
    LorStringView remaining = {
        .data = input.data + comma + 1u,
        .size = input.size - comma - 1u,
    };

    fwrite(first.data, 1, first.size, stdout);
    printf(" | ");
    fwrite(remaining.data, 1, remaining.size, stdout);
    putchar('\n');

    // LorString is a char * typedef, so plain NULL initialization is valid.
    char *text = NULL;
    if (lor_string_append(&text, first) != LOR_STATUS_OK ||
        lor_string_append_cstr(&text, " owns its buffer") != LOR_STATUS_OK) {
        lor_string_deinit(&text);
        return 1;
    }

    // The content pointer supports normal indexing and read-only C string APIs.
    text[0] = 'A';
    puts(text);
    printf("strlen=%zu, stored size=%zu\n", strlen(text),
           lor_string_size(text));

    // Directly built views into an owned string are valid until it may grow.
    LorStringView word = {
        .data = text,
        .size = first.size,
    };
    fwrite(word.data, 1, word.size, stdout);
    putchar('\n');

    // The hidden header remains private; release the handle through liblor.
    lor_string_deinit(&text);
    return text != NULL;
}
