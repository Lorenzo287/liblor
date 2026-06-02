#if defined(WIN32)
    #include "libmill.h"
#elif defined(__linux__)
    #include <libmill.h>
#endif
#include <stdio.h>

coroutine void handle_client(tcpsock s) {
    char buf[256];
    printf("Client connected\n");
    while (1) {
        ssize_t n = tcprecv(s, buf, sizeof(buf), -1);
        if (n <= 0) break;
        printf("Received from client: \n%.*s\n", (int)n, buf);
        tcpsend(s, buf, n, -1);
    }
    tcpclose(s);
    printf("Client disconnected\n");
}

coroutine void server(int port) {
    ipaddr addr = iplocal(NULL, port, 0);
    tcpsock listener = tcplisten(addr, 10);
    printf("Server listening on port %d\n", port);
    while (1) {
        tcpsock s = tcpaccept(listener, -1);
        go(handle_client(s));
    }
}

int main(void) {
    go(server(5555));
    // Keep main alive forever
    while (1) { msleep(now() + 1000); }
}
