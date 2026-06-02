#include <stdio.h>

#define FOR_LIST(DO) \
    DO(id1, name1)   \
    DO(id2, name2)   \
    DO(id3, name3)

#define DEFINE_NAME_VAR(id, name, ...) int name = 69;
#define PRINT_NAME_AND_VALUE(id, name, ...) printf(#name " = %d\n", name);
#define DEFINE_ENUMERATION(id, name, ...) name = 420,

enum IdListType { FOR_LIST(DEFINE_ENUMERATION) };

int main(void) {
    FOR_LIST(PRINT_NAME_AND_VALUE)
    printf("\n");

    FOR_LIST(DEFINE_NAME_VAR);
    FOR_LIST(PRINT_NAME_AND_VALUE)
    return 0;
}
