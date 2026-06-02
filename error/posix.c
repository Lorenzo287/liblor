#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

/* MinGW: unistd.h exists but pipe() is missing — use _pipe() from io.h */
#ifdef _WIN32
    #include <io.h>
    #include <unistd.h>
#else
    #include <unistd.h>
#endif

/* MinGW: S_IRUSR may be absent, S_IREAD/S_IWRITE are always available */
#ifdef _WIN32
    #define MODE_RO S_IREAD
    #define MODE_RW (S_IREAD | S_IWRITE)
#else
    #define MODE_RO S_IRUSR
    #define MODE_RW (S_IRUSR | S_IWUSR)
#endif

/* pipe: MinGW needs _pipe(fds, bufsize, mode); POSIX just pipe(fds).
   POSIX also raises SIGPIPE on a broken-pipe write — Windows doesn't,
   the write just fails with EPIPE directly. */
static void demo_pipe(void) {
    int fds[2];
#ifdef _WIN32
    if (_pipe(fds, 256, O_BINARY) == -1) {
        perror("_pipe");
        return;
    }
#else
    if (pipe(fds) == -1) {
        perror("pipe");
        return;
    }
    signal(SIGPIPE, SIG_IGN);
#endif
    close(fds[0]);
    if (write(fds[1], "x", 1) == -1) perror("write broken pipe");
    close(fds[1]);
#ifndef _WIN32
    signal(SIGPIPE, SIG_DFL);
#endif
}

/* strerror_r vs strerror_s: same idea, different arg order and return type.
   GNU adds a third variant that returns char* instead of writing into buf. */
static void demo_strerror(int errnum) {
    char buf[128];
#ifdef _WIN32
    /* strerror_s(buf, size, errnum) — C11 Annex K */
    if (strerror_s(buf, sizeof buf, errnum) != 0)
        snprintf(buf, sizeof buf, "unknown error %d", errnum);
#elif defined(_GNU_SOURCE)
    /* GNU strerror_r returns char*, may ignore buf entirely */
    char *msg = strerror_r(errnum, buf, sizeof buf);
    fprintf(stderr, "strerror_r: %s\n", msg);
    return;
#else
    /* POSIX XSI strerror_r writes into buf, returns int */
    if (strerror_r(errnum, buf, sizeof buf) != 0)
        snprintf(buf, sizeof buf, "unknown error %d", errnum);
#endif
    fprintf(stderr, "strerror: %s\n", buf);
}

/* chmod vs _chmod — everything else (open, fopen, remove) is portable */
static void demo_permissions(void) {
    const char *path = "rdonly.tmp";
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, MODE_RO);
    if (fd != -1) close(fd);
    if (fopen(path, "w") == NULL) perror("fopen read-only");
#ifdef _WIN32
    _chmod(path, MODE_RW);
#else
    chmod(path, MODE_RW);
#endif
    remove(path);
}

/* saving/restoring errno is fully portable — no ifdefs needed */
static void demo_save_errno(void) {
    if (fopen("nope.txt", "r") == NULL) {
        int saved = errno;
        close(-1); /* clobbers errno */
        errno = saved;
        perror("fopen");
    }
}

int main(void) {
    demo_pipe();
    demo_strerror(ENOENT);
    demo_permissions();
    demo_save_errno();
    return 0;
}
