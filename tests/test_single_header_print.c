// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS
#define LOR_IMPLEMENTATION
#define LOR_ENABLE_PRINT
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr)                          \
    do {                                     \
        if (!(expr)) {                       \
            fprintf(stderr, "check failed"); \
            return 1;                        \
        }                                    \
    } while (0)

static int file_equals(FILE *file, const char *expected) {
    char buffer[64];
    if (fflush(file) != 0 || fseek(file, 0, SEEK_SET) != 0) return 0;
    size_t size = fread(buffer, 1, sizeof(buffer) - 1u, file);
    buffer[size] = '\0';
    return strcmp(buffer, expected) == 0;
}

int main(void) {
    CHECK(strcmp(type_name(1.0), "double") == 0);
    StringView view = SV_LITERAL("view");
    CHECK(strcmp(type_name(view), "LorStringView") == 0);

    FILE *file = tmpfile();
    CHECK(file != NULL);
    int *numbers = ARRAY_INIT;
    CHECK(array_push_as(numbers, int, 4) == STATUS_OK);
    CHECK(array_push_as(numbers, int, 8) == STATUS_OK);
    CHECK(fprint(file, "single", print_array(numbers), end("")) == STATUS_OK);
    CHECK(file_equals(file, "single [4, 8]"));
    array_deinit(&numbers);
    CHECK(fclose(file) == 0);

    puts("test_single_header_print: ok");
    return 0;
}
