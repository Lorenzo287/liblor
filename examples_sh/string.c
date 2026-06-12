#include <stdio.h>

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_STRING
#define LOR_LEAKCHECK
#include "../lor.h"

#define SV(str) LOR_SV_LITERAL(str)

int main(void) {
    LorString str1 = LOR_STRING_INIT;
    lor_string_assign_cstr(&str1, "owned string");

    // NOTE: LorString is an alias for char *,
    // liblor string are completely compatible with c strings
    // (the only thing you cannot do is call free() on them)
    char *str2 = NULL;
    // can also assign from a string view
    lor_string_assign(&str2, (LorStringView)SV("hello"));

    LOR_AUTO_STRING char *str3 = NULL;
    // when the string is empty append and assign are equivalent
    lor_string_append_cstr(&str3, "foobar");

    // can interact like with c strings
    str1[0] = 'O';
    str2[0] = 'H';
    str3[0] = 'F';
    printf("%s, %s, %s\n", str1, str2, str3);

    size_t size = strlen(str1) + lor_string_size(str2) + strlen(str3);
    LorString big_str = NULL;
    lor_string_reserve(&big_str, size + 2);
    // now the append will not allocate, improving performace
    lor_string_append_cstr(&big_str, str1);
    lor_string_append_char(&big_str, '-');
    lor_string_append_cstr(&big_str, str2);
    lor_string_append_char(&big_str, '-');
    lor_string_append_cstr(&big_str, str3);
    printf("%s\n", big_str);

    lor_string_deinit(&str1);
    lor_string_deinit(&str2);
    // no need to free str3 which is managed automatically

    return 0;
}
