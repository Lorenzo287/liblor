// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_STRING
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>
#include <string.h>

#include "lor_test.h"

int main(void) {
    StringView input = sv_from_cstr("name=liblor");
    StringView key;
    StringView value;
    CHECK(sv_split_char(input, '=', &key, &value));
    CHECK(sv_equal(key, ((StringView)SV_LITERAL("name"))));
    CHECK(sv_equal(value, ((StringView)SV_LITERAL("liblor"))));

    String string = STRING_INIT;
    CHECK(string_append(&string, value) == STATUS_OK);
    CHECK(string_append_cstr(&string, " string") == STATUS_OK);
    CHECK(strcmp(string, "liblor string") == 0);
    CHECK(string_size(string) == 13);
    string_deinit(&string);

    puts("test_single_header_string: ok");
    return 0;
}
