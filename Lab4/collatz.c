#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define ull unsigned long long

// strtoull but only reading valid positive integers, prints an warning to stderr and returns 0 for errors
ull checked_strtoull(char *input) {
    char *parse_result;
    errno = 0;
    ull collatz = strtoull(input, &parse_result, 10);
    if(*parse_result != '\0' || errno == ERANGE || strchr(input, '-')) {
        fprintf(stderr, "Failed to parse input '%s' as unsigned long long\n", input);
        return 0;
    }
    if(collatz == 0) {
        fprintf(stderr, "Input must be strictly positive\n");
    }
    return collatz;
}

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Usage: collatz <number>\n");
        return EXIT_FAILURE;
    }

    ull collatz = checked_strtoull(argv[1]);
    if(collatz == 0) {
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
