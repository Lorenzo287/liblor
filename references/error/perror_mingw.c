#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <io.h>       /* _pipe() */
#include <sys/stat.h>

int main(void)
{
    char buf[64];
    int pipefd[2];

    /* ENOENT — file not found */
    fopen("nope.txt", "r");
    perror("fopen");

    /* EACCES — permission denied */
    int fd = open("rdonly.tmp", O_CREAT | O_WRONLY | O_TRUNC, S_IREAD);
    close(fd);
    fopen("rdonly.tmp", "w");
    perror("fopen read-only");
    _chmod("rdonly.tmp", S_IREAD | S_IWRITE);
    remove("rdonly.tmp");

    /* EEXIST — exclusive create on existing file */
    fd = open("exist.tmp", O_CREAT | O_WRONLY, S_IREAD | S_IWRITE);
    close(fd);
    open("exist.tmp", O_CREAT | O_EXCL | O_WRONLY, S_IREAD | S_IWRITE);
    perror("open O_EXCL");
    remove("exist.tmp");

    /* EBADF — bad file descriptor */
    read(999, buf, sizeof buf);
    perror("read");

    /* EINVAL — invalid argument */
    FILE *fp = fopen("seek.tmp", "w+");
    fseek(fp, 0, 99);
    perror("fseek");
    fclose(fp);
    remove("seek.tmp");

    /* EPIPE — write to broken pipe (no SIGPIPE on Windows) */
    _pipe(pipefd, 256, O_BINARY);
    close(pipefd[0]);
    write(pipefd[1], "x", 1);
    perror("write broken pipe");
    close(pipefd[1]);

    /* ERANGE — arithmetic overflow */
    errno = 0;
    char *end;
    strtol("99999999999999999999999", &end, 10);
    perror("strtol");

    return 0;
}
