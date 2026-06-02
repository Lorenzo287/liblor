#include <Cello.h>

int main(int argc, char **argv) {
    help(Range);
    foreach (i in range($I(10))) print("%$ ", i);
    return 0;
}
