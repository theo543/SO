#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "collatz_child.h"

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Usage: collatz <number>\n");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if(pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    } else if(pid == 0) {
        exit(collatz_child(argv[1]));
    } else {
        // parent
        if(wait(NULL) == -1) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
        printf("Child %d finished\n", pid);
    }
}
