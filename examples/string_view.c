#include "lor/string.h"

#include <stdio.h>

int main(void) {
    LorStringView fields = lor_sv_from_cstr(" language = C ; project = liblor ");

    while (1) {
        LorStringView field;
        int more = lor_sv_chop_char(&fields, ';', &field);

        LorStringView key;
        LorStringView value;
        if (lor_sv_split_once_char(field, '=', &key, &value)) {
            key = lor_sv_trim(key);
            value = lor_sv_trim(value);
            lor_sv_print(key);
            printf(" -> ");
            lor_sv_print(value);
            putchar('\n');
        }
        if (!more) break;
    }
	printf("\n");

#define SV(text) (LorStringView)LOR_SV_LITERAL(text)

	LorStringView comma_separated = SV("ciao,come,va");
	while (1) {
		LorStringView word;
		int found_separator = lor_sv_chop_char(&comma_separated, ',', &word);
		lor_sv_print(word);
		putchar('\n');
		if (!found_separator) break;
	}
    return 0;
}
