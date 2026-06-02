#ifndef CLEANUP_H
#define CLEANUP_H

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>  // needed for linux

#define _cleanup_(x) __attribute__((__cleanup__(x)))

#define TAKE_PTR(ptr)        \
    ({                       \
        void *__ptr = (ptr); \
        (ptr) = NULL;        \
        __ptr;               \
    })

#define TAKE_FD(fd)      \
    ({                   \
        int __fd = (fd); \
        (fd) = -EBADF;   \
        __fd;            \
    })

#if defined(POSIX)
static inline void unlock_mutex(pthread_mutex_t **mutex) {
    if (*mutex) pthread_mutex_unlock(*mutex);
}
    #define _cleanup_unlock_ _cleanup_(unlock_mutex)
#endif

static inline void unlink_temp(char **path) {
    if (*path) {
        unlink(*path);
        free(*path);
    }
}
#define _cleanup_temp_ _cleanup_(unlink_temp)

static inline void closep(int *fd) {
    if (*fd >= 0) close(*fd);
}
#define _cleanup_close_ _cleanup_(closep)

static inline void freep(void *p) {
    free(*(void **)p);
}
#define _cleanup_free_ _cleanup_(freep)

#endif  // CLEANUP_H
