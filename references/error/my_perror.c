#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef enum {
    ERR_OK = 0,
    ERR_BAD_INPUT = 1,
    ERR_OVERFLOW = 2,
    ERR_NOT_FOUND = 3,
} MyError;

MyError my_errno;

const char *my_strerror(MyError e) {
    switch (e) {
    case ERR_OK:
        return "success";
    case ERR_BAD_INPUT:
        return "bad input";
    case ERR_OVERFLOW:
        return "value out of range";
    case ERR_NOT_FOUND:
        return "entry not found";
    default:
        return "unknown error";
    }
}

void my_perror(const char *prefix) {
    fprintf(stderr, "%s: %s\n", prefix, my_strerror(my_errno));
}

int parse_age(const char *s, int *out) {
    if (!s) { return ERR_BAD_INPUT; }
    char *end;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno || end == s) return ERR_BAD_INPUT;
    if (v < 0 || v > 150) return ERR_OVERFLOW;
    *out = (int)v;
    return ERR_OK;
}

int main(void) {
    int age;

    my_errno = parse_age("999", &age);
    if (my_errno) my_perror("parse_age");  // parse_age: value out of range

    my_errno = parse_age(NULL, &age);
    if (my_errno) my_perror("parse_age");  // parse_age: bad input
}
