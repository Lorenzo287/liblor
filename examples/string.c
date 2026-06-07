#include "lor/string.h"

#include <stdio.h>

int main(void) {
    LorString message = LOR_STRING_INIT;

    if (lor_string_append_cstr(&message, "hello") != LOR_STATUS_OK ||
        lor_string_append_char(&message, ' ') != LOR_STATUS_OK ||
        lor_string_append_cstr(&message, "from liblor") != LOR_STATUS_OK) {
        lor_string_deinit(&message);
        return 1;
    }

    puts(message);
    printf("size=%zu capacity=%zu\n", lor_string_size(message),
           lor_string_capacity(message));

    lor_string_deinit(&message);
    return 0;
}
