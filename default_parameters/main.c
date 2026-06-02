#include <stdio.h>
#include <stdbool.h>

typedef struct {
    int width;
    int height;
    bool fullscreen;
    bool resizable;
} WindowParams;

void _CreateWindow(const char *title, WindowParams params) {
    printf("Title: %s\n", title);
    printf("Width: %d\n", params.width);
    printf("Height: %d\n", params.height);
    printf("Fullscreen: %d\n", params.fullscreen);
    printf("Resizable: %d\n", params.resizable);
}

// may cause lsp warning since struct fields are redefined
#define CreateWindow(title, ...)                               \
    _CreateWindow((title), (WindowParams){.width = 800,        \
                                          .height = 600,       \
                                          .fullscreen = false, \
                                          .resizable = true,   \
                                          __VA_ARGS__})

// NOTE: this does not work since if we want to set a param to 0 it will fail,
// for example we could never set to false a boolean that is true by default
// (this approach could be used if we know our variable is never 0)
#define CreateWindowWrong(title, ...)                                       \
    do {                                                                    \
        WindowParams params = {.width = 800,                                \
                               .height = 600,                               \
                               .fullscreen = false,                         \
                               .resizable = true};                          \
        WindowParams overrides = {__VA_ARGS__};                             \
        if (overrides.width) params.width = overrides.width;                \
        if (overrides.height) params.height = overrides.height;             \
        if (overrides.fullscreen) params.fullscreen = overrides.fullscreen; \
        if (overrides.resizable) params.resizable = overrides.resizable;    \
        _CreateWindow((title), params);                                     \
    } while (0)

int main(void) {
    // omit all default params
    CreateWindow("Window1");
    printf("\n");

    // override defaults
    CreateWindow("Window2", .width = 1280, .height = 720);
    printf("\n");
    CreateWindow("Window3", .resizable = false);
    return 0;
}
