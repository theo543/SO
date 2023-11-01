#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

#define ull unsigned long long

// strtoull but only reading valid positive integers, prints an warning to stderr and returns 0 for errors
ull checked_strtoull(char *input) {
    char *parse_result;
    errno = 0;
    ull collatz = strtoull(input, &parse_result, 10);
    if(*parse_result != '\0' || errno == ERANGE || collatz == 0 || strchr(input, '-')) {
        fprintf(stderr, "Failed to parse input '%s' as positive unsigned long long\n", input);
        return 0;
    }
    return collatz;
}

int child(char* input) {
    ull collatz = checked_strtoull(input);
    if(collatz == 0) {
        return EXIT_FAILURE;
    }
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
    pid_t me = getpid();
    pid_t parent = getppid();
    printf("Done Parent %d Me %d\n", parent, me);
    return EXIT_SUCCESS;
}

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
            exit(child(argv[x]));
        } else {
            // parent
            continue;
        }
    }
    for(pid_t pid;(pid = wait(NULL)) != -1;) {
        // wait all processes
    }
    return EXIT_SUCCESS;
}
