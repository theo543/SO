#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>

const char *SHM_NAME = "SHMCOLLATZ_SHARED_MEMORY_SEGMENT";
const char *SUBPROCESS_FLAG = "-shm";

int main_subprocess(int argc, char **argv) {
    // TODO: create shared memory, parse args, spawn subprocesses
    return EXIT_SUCCESS;
}

int main_mainprocess(int argc, char **argv) {
    // TODO: connect to shared memory, receive offset, process one number
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    if(argc <= 1) {
        printf("Usage: shmcollatz <nr_1 nr_2 ... nr_n> | <%s (implementation detail) [offset via stdin]>\n", SUBPROCESS_FLAG);
        return EXIT_FAILURE;
    } else if(argc == 2 && strcmp(argv[1], SUBPROCESS_FLAG)) {
        exit(main_subprocess(argc, argv));
    } else {
        exit(main_mainprocess(argc, argv));
    }
}
