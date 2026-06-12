#include <stdio.h>

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_TYPE
#define LOR_ENABLE_STRING
#include "../lor.h"

int main(void) {
    printf("%s\n", lor_type_name(42));        // int
    printf("%s\n", lor_type_name(3.14));      // double
    printf("%s\n", lor_type_name("liblor"));  // C string
	
	void *ptr = NULL;
	// LorTypeKind is just an enum
    LorTypeKind type = lor_type_kind(ptr);
    // this function is simply a switch between enums
	const char *type_name = lor_type_kind_name(type);
	printf("\n%s\n", type_name);

	// the same can be done in a single step
    printf("%s\n", lor_type_name(ptr));

	// NOTE: some of liblor components are part of the types,
	// others are not because they are standard c types
	// (for example LorString is a char *, while array and maps
	// are typed pointers so they don't have a dedicated type)
	LorStringView sv = LOR_STRING_VIEW_INIT;
    printf("\n%s\n", lor_type_name(sv));
    return 0;
}
