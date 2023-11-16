#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "macros.h"

struct strrev_thread_args {
    const char *string;
    char **return_return;
};

void * strrev_thread(void *args_void) {
    struct strrev_thread_args *args = args_void;
    int len = strlen(args->string);
    char *reversed = malloc(len + 1);
    if(reversed == NULL) {
        return NULL;
    }
    for(int x = 0, y = len - 1;x < len;x++, y--) {
        reversed[y] = args->string[x];
    }
    reversed[len] = '\0';
    *args->return_return = reversed;
    return NULL;
}

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Usage: strrev STRING\n");
        return EXIT_FAILURE;
    }
    char *result = NULL;
    struct strrev_thread_args args = {
        argv[1],
        &result
    };
    pthread_t thr;
    PT_CALL(pthread_create(&thr, NULL, strrev_thread, &args));
    PT_CALL(pthread_join(thr, NULL));
    if(result == NULL) {
        fprintf(stderr, "Unknown error in strrev_thread\n");
        return EXIT_FAILURE;
    }
    printf("%s\n", result);
    return 0;
}
