#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

const char *LS_LOCATION = "/usr/bin/ls";

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Usage: fibonacci <number>\n");
        return EXIT_FAILURE;
    }

    char *parse_result;
    errno = 0;
    unsigned long long collatz = strtoull(argv[1], &parse_result, 10);
    if(*parse_result != '\0' || errno == ERANGE || strchr(argv[1], '-')) {
        fprintf(stderr, "Failed to parse input as unsigned long long\n");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if(pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    } else if(pid == 0) {
        // child
        printf("%llu: %llu", collatz, collatz);
        while(collatz != 1ull) {
            if((collatz % 2ull) == 0) {
                collatz /= 2ull;
            } else {
                if((collatz >= (ULLONG_MAX / 3ull)) || (collatz * 3ull == ULLONG_MAX)) {
                    printf(" <OVERFLOW: %llu * 3 + 1 does not fit in unsigned long long>\n", collatz);
                    return EXIT_FAILURE;
                }
                collatz = collatz * 3 + 1;
            }
            printf(" %llu", collatz);
        }
        printf("\n");
    } else {
        // parent
        if(wait(NULL) == -1) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
        printf("Child %d finished\n", pid);
    }
}
