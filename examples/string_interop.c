#include "lor/string.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    // C string -> borrowed view.
    const char *config = "project = liblor ; language = C";
    LorStringView input = lor_sv_from_cstr(config);

    LorStringView project_field;
    lor_sv_chop_char(&input, ';', &project_field);

    LorStringView key;
    LorStringView value;
    if (!lor_sv_split_once_char(project_field, '=', &key, &value)) return 1;

    key = lor_sv_trim(key);
    value = lor_sv_trim(value);

    // Views may not end at a NUL byte, so print them using their exact lengths.
    lor_sv_print(key);
    printf(" -> ");
    lor_sv_print(value);
    putchar('\n');

    // Borrowed view -> owned dynamic string.
    LorString message = LOR_STRING_INIT;
    if (lor_string_init_view(&message, value) != LOR_STATUS_OK ||
        lor_string_append_cstr(&message, " library") != LOR_STATUS_OK) {
        lor_string_deinit(&message);
        return 1;
    }

    // A non-NULL LorString is directly compatible with read-only C string APIs.
    printf("C string: %s\n", message);
    printf("strlen: %zu, stored size: %zu\n", strlen(message),
           lor_string_size(message));

    // Owned string -> borrowed view, with allocation-free slicing.
    LorStringView message_view = lor_string_view(message);
    LorStringView first_word = lor_sv_take_left(message_view, value.size);
    printf("view of owned string: ");
    lor_sv_print(first_word);
    putchar('\n');

    lor_string_deinit(&message);
    return 0;
}
