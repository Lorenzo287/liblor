#include <stdio.h>
#include <stdbool.h>

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_STRING
#include "../lor.h"

#define SV(str) LOR_SV_LITERAL(str)

// NOTE: use string views when you want to visualize (part of) a string in
// some particular way or interact with it non destructively.
// You cannot use them to create copies or append to a string.

int main(void) {
    // all valid initializations,
    // in general liblor gives transparent access to struct
    LorStringView sv1 = {"Hello, World!", 13};
    LorStringView sv2 = SV("Hello, World!");
    LorStringView sv3 = lor_sv_from_cstr("Hello, World!");
    LorStringView sv4 = sv3; (void)sv4;

	// can also create a view from a LorString
	char *str = NULL;
	lor_string_assign_cstr(&str, "owned string");
	LorStringView sv5 = lor_string_view(str); (void)sv5;

    // WARN: this is not good practice since the position of the terminator
    // might not concide with the intended length of the sv
    if (0) printf("%s\n", sv1.data);

    // these are valid solutions
    printf("--- PRINT ALTERNATIVES ---\n");
    lor_sv_print(sv1);
    printf("\n");
    printf("%.*s\n", (int)sv1.size, sv1.data);
    fwrite(sv1.data, sv1.size, 1, stdout);
    printf("\n");

    printf("\n--- MANIPULATION MAGIC ---\n");
    if (lor_sv_starts_with(sv2, (LorStringView)SV("Hello"))) {
        LorStringView part1, part2;
        lor_sv_split_char(sv2, ',', &part1, &part2);
        part2 = lor_sv_slice(part2, 1, 5);  // _World! -> World
        lor_sv_print(part2);
        printf("\n");
    }

    printf("\n--- ITERATION OVER DELIMITERS ---\n");
    LorStringView sv = SV("ciao, come, stai, tutto, bene");
    LorStringView chopped;
    while (1) {
		bool chop = lor_sv_chop_char(&sv, ',', &chopped);
        lor_sv_print(lor_sv_trim(chopped));
        printf("\n");
		if (!chop) break;
    }

    return 0;
}
