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
    if(*parse_result != '\0' || errno == ERANGE || strchr(input, '-')) {
        fprintf(stderr, "Failed to parse input '%s' as unsigned long long\n", input);
        return 0;
    }
    if(collatz == 0) {
        fprintf(stderr, "Input must be strictly positive\n");
    }
    return collatz;
}

void child(ull collatz) {
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
}

// returns true if succesfully forked a collatz process
bool try_fork(char* input) {
    ull collatz = checked_strtoull(input);
    if(collatz == 0) {
        return false;
    }

    pid_t pid = fork();
    if(pid < 0) {
        perror("fork");
    } else if(pid == 0) {
        child(collatz);
    } else {
        // parent
        return true;
    }
}

int main(int argc, char **argv) {
    if(argc == 1) {
        fprintf(stderr, "Usage: ncollatz <number>[1, +inf)\n");
        return EXIT_FAILURE;
    }
    int sucessful_forks = 0;
    for(int x = 1;x < argc;x++) {
        sucessful_forks += (int)try_fork(argv[x]);
    }
    for(pit_t pid;pid = wait(NULL);) {
        
    }
    if(sucessful_forks != (argc - 1)) {
        fprintf(stderr, "Warning: failed to start some processes. Started %d instead of %d.", sucessful_forks, (argc - 1));
    }
}
