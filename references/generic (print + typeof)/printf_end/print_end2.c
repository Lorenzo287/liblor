#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// NOTE: printf with optional end("") as FIRST argument, 
// uses a clever marker

#define MARKER "\0LORENZO"
#define MARKER_LEN 8
#define end(s) MARKER s

void print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    const char *end_str = "\n";  // default
    if (strncmp(fmt, MARKER, MARKER_LEN) == 0) {
        end_str = fmt + MARKER_LEN;
        fmt = va_arg(args, const char *);
    }
    vprintf(fmt, args);
    printf("%s", end_str);

    va_end(args);
}

int main(void) {
    char *s = "hello";

    print("|%s with newline|", s);
    print(end("!\n"), "|%s with custom ending|", s);
    print(end(""), "|%d %f|", 10, 3.14);
    print(end(""), "|no newline before this|");
    print("|hello|");

    return 0;
}
