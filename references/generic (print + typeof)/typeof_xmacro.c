#include <stdio.h>

// NOTE: xmacro helper that could be used to extend the type macro
// to custom structs

#define TYPE_LIST(X)         \
    X(int, "int")            \
    X(float, "float")        \
    X(Point, "struct Point") \
    X(Rect, "struct Rect")

#define GENERIC_CASE(type, name) \
    type:                        \
    name,

#define type(x) _Generic((x), TYPE_LIST(GENERIC_CASE) default: "unknown")

// custom typedefs
typedef struct {
    int x, y;
} Point;
typedef struct {
    int w, h;
} Rect;

int main(void) {
    Point p;
    Rect r;
    int a = 5;

    printf("%s\n", type(p));
    printf("%s\n", type(r));
    printf("%s\n", type(a));

    return 0;
}
