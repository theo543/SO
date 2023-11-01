#ifndef COLLATZ_CHILD_H
#define COLLATZ_CHILD_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

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

int collatz_child(char* input) {
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
    printf(".\n");

    return EXIT_SUCCESS;
}

#endif
