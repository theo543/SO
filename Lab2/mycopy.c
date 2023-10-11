#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char** args) {
    if(argc != 3) {
        static const char err[] = "Usage: syscopy <src> <dst>\n";
        write(2, err, sizeof(err));
        return 1;
    }

    const char *src = args[1];
    const char *dst = args[2];

    struct stat st;
    if(stat(src, &st)) {
        perror("stat source file");
        return errno;
    }

    int srcfd = open(src, O_RDONLY);
    if(srcfd == -1) {
        perror("open source file");
        return errno;
    }

    char *buffer = malloc(st.st_size);
    if(buffer == NULL) {
        perror("allocate");
        return errno;
    }

    size_t bytesread = 0, byteswritten = 0;
    while(bytesread < st.st_size) {
        int ret = read(srcfd, buffer + bytesread, st.st_size - bytesread);
        if(ret == -1) {
            perror("reading");
            return errno;
        }
        bytesread += ret;
    }

    if(close(srcfd)) {
        perror("close source");
        return errno;
    }

    int dstfd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(dstfd == -1) {
        perror("open destination file");
        return errno;
    }

    while(byteswritten < st.st_size) {
        int ret = write(dstfd, buffer + byteswritten, st.st_size - byteswritten);
        if(ret == -1) {
            perror("writing");
            return errno;
        }
        byteswritten += ret;
    }

    free(buffer);

    if(close(dstfd)) {
        perror("close destination");
        return errno;
    }

    return 0;
}
