#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "collatz_child.h"

int main(int argc, char **argv) {
    if(argc == 1) {
        fprintf(stderr, "Usage: ncollatz <number>[1, +inf)\n");
        return EXIT_FAILURE;
    }
    for(int x = 1;x < argc;x++) {
        pid_t result = fork();
        if(result == -1) {
            perror("fork");
        } else if(result == 0) {
            // child
            int retcode = collatz_child(argv[x]);
            pid_t me = getpid();
            pid_t parent = getppid();
            printf("Done Parent %d Me %d\n", me, parent);
            exit(retcode);
        } else {
            // parent
            continue;
        }
    }
    for(pid_t pid;(pid = wait(NULL)) != -1;) {
        // wait all processes
    }
}
