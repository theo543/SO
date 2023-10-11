#include <unistd.h>
#include <errno.h>
int main(void) {
    static const char msg[] = "Hello, world!\n";
    const char *buf = msg;
    size_t left = sizeof(msg) - 1;
    while(left > 0) {
        ssize_t ret = write(1, buf, left);
        if(ret < 0) {
            return errno;
        }
        left -= ret;
        buf += ret;
    }
    return 0;
}
