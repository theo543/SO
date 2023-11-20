#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "macros.h"

void * strrev_thread(void *str_void) {
    const char *str = str_void;
    int len = strlen(str);
    char *reversed = malloc(len + 1);
    if(reversed == NULL) {
        return NULL;
    }
    for(int x = 0, y = len - 1;x < len;x++, y--) {
        reversed[y] = str[x];
    }
    reversed[len] = '\0';
    return reversed;
}

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Usage: strrev STRING\n");
        return EXIT_FAILURE;
    }
    pthread_t thr;
    PT_CALL(pthread_create(&thr, NULL, strrev_thread, argv[1]));
    void *result_void;
    PT_CALL(pthread_join(thr, &result_void));
    if(result_void == NULL) {
        fprintf(stderr, "Unknown error in strrev_thread\n");
        return EXIT_FAILURE;
    }
    char *result = result_void;
    printf("%s\n", result);
    free(result);
    return 0;
}
