#include <stdio.h>

#define LIST   \
    X(var1, 5) \
    X(var2, 9) \
    X(var3, 2)

int main(void) {
    
#define X(var, num) int var = num;
    LIST
#undef X

#define X(var, num) printf("%s: %d\n", #var, var);
    LIST
#undef X

	return 0;
}
