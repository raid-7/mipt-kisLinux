#include <unistd.h>
#include <errno.h>

char safe_read(int fd, void* buf, size_t len, void (*error_handler) (const char*)) {
    size_t ready = 0;
    while (ready < len) {
        ssize_t actually_got = read(fd, buf + ready, len - ready);
        char err = errno != 0;
        error_handler("IO error");
        if (err || !actually_got)
            return 0;
        ready += actually_got;
    }
    return 1;
}

char safe_write(int fd, void* buf, size_t len, void (*error_handler) (const char*)) {
    size_t ready = 0;
    while (ready < len) {
        ssize_t actually_put = write(fd, buf + ready, len - ready);
        char err = errno != 0;
        error_handler("IO error");
        if (err)
            return 0;
        ready += actually_put;
    }
    return 1;
}
