// C stdlib
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
// Linux
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>

const char *SHM_NAME = "/SHMCOLLATZ_SHARED_MEMORY_SEGMENT";
const char *SUBPROCESS_FLAG = "-shm";
const int NUMBERS_FOR_COLLATZ = 3000; // up to 3000 numbers in the sequence
const int NUMBERS_FOR_METADATA = 2; // the collatz numbers are terminated by a 0, then an error code follows
const int NUMBERS_FOR_INPUT = 10; // always at the end of the memory, 40 chars, more than enough for inputs that fit in 64 bits
const int TOTAL_NUMBERS_PER_PROCESS = NUMBERS_FOR_COLLATZ + NUMBERS_FOR_METADATA + NUMBERS_FOR_INPUT;
const int MEMORY_PER_PROCESS = TOTAL_NUMBERS_PER_PROCESS * sizeof(uint64_t);

// used by subprocess to report errors
const uint64_t ERROR_NONE = 0;
const uint64_t ERROR_PARSE_FAILED = 1;
const uint64_t ERROR_INTEGER_OVERFLOW = 2;
const uint64_t ERROR_SHARED_BUFFER_OVERFLOW = 3;
const uint64_t ERROR_NO_RESPONSE = 4;
const char *ERROR_MSG[] = {
    "No error. (should never be printed)",
    "Failed to parse input as positive integer.",
    "Integer overflow.",
    "Out of space to store results.",
    "Did not receive data from subprocess."
};

// strtoull but only reading valid positive integers, returns 0 for errors
uint64_t checked_strtoull(char *input) {
    char *parse_result;
    errno = 0;
    uint64_t collatz = strtoull(input, &parse_result, 10);
    if(*parse_result != '\0' || errno == ERANGE || collatz == 0 || strchr(input, '-')) {
        return 0;
    }
    return collatz;
}

int main_mainprocess(int argc, char **argv, char **envp) {
    printf("Starting Parent %d\n", getpid());

    int procs = argc - 1;
    int pagesize = getpagesize();
    int minimum_mem_size = procs * MEMORY_PER_PROCESS;
    // round up to pagesize multiple
    int shm_size = (minimum_mem_size / pagesize) * pagesize;
    if(minimum_mem_size % pagesize != 0) {
        shm_size += pagesize;
    }

    int shm_fd = shm_open(SHM_NAME, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
    if(shm_fd < 0) {
        perror("shm_open");
        return errno;
    }
    if(ftruncate(shm_fd, shm_size) == -1) {
        perror("ftruncate");
        return errno;
    }
    uint64_t *shm_ptr = mmap(0, shm_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm_ptr == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        return errno;
    }

    int self_location_fd = open("/proc/self/exe", O_RDONLY);
    if(self_location_fd == -1) {
        perror("open(\"/proc/self/exe\")");
        munmap(shm_ptr, shm_size);
        shm_unlink(SHM_NAME);
    }

    for(int x = 1;x<argc;x++) {
        // send offset to subprocess via argument
        char offset_str[50];
        int subprocess_offset = TOTAL_NUMBERS_PER_PROCESS * (x - 1);
        sprintf(offset_str, "%d", subprocess_offset);

        uint64_t *subproc_shm = shm_ptr + subprocess_offset;
        *subproc_shm = 0;
        *(subproc_shm + 1) = ERROR_NO_RESPONSE;

        // since atoi skips any amount of whitespace, don't send any of it to the child
        char* input_copy_start = argv[x];
        while(isspace(*input_copy_start)) {
            input_copy_start++;
        }

        char *subproc_shm_input = (char*)(subproc_shm + NUMBERS_FOR_COLLATZ + NUMBERS_FOR_METADATA);
        *subproc_shm_input = '\0';
        strncat(subproc_shm_input, input_copy_start, sizeof(uint64_t) * NUMBERS_FOR_INPUT - 1);

        // args to pass to subprocess (null-terminated)
        // fexecve is missing a const qualifier because of backward compatibility
        // cast away SUBPROCESS_FLAG const
        char * new_argv[] = {argv[0], (char*)SUBPROCESS_FLAG, offset_str, NULL};

        pid_t pid = fork();
        if(pid == -1) {
            perror("fork");
        } else if(pid == 0) {
            // child
            fexecve(self_location_fd, new_argv, envp);

            // fexecve should not return
            perror("fexecve");
            return EXIT_FAILURE;
        } else {
            // parent
            // don't do anything yet
        }
    }

    for(;;) {
        pid_t pid = wait(NULL);
        if(pid == -1) {
            break;
        }
    }

    for(int x = 1;x<argc;x++) {
        printf("%s:", argv[x]);
        uint64_t *shm_iter = shm_ptr + TOTAL_NUMBERS_PER_PROCESS * (x - 1);
        while(*shm_iter != 0) {
            printf(" %ld", *shm_iter);
            shm_iter++;
        }
        shm_iter++;
        if(*shm_iter != ERROR_NONE) {
            printf(" <%s>", ERROR_MSG[*shm_iter]);
        }
        printf("\n");
    }

    munmap(shm_ptr, shm_size);
    shm_unlink(SHM_NAME);
    close(self_location_fd);
    return EXIT_SUCCESS;
}

int main_subprocess(char **argv) {
    int retcode = 0;
    int shm_fd = shm_open(SHM_NAME, O_RDWR, S_IRUSR|S_IWUSR);
    if(shm_fd < 0) {
        perror("shm_open (subprocess)");
        retcode = errno;
        goto exit;
    }

    // get offset
    int offset = atoi(argv[2]);
    // not really the full size, but we don't need that
    int shm_size = offset * sizeof(uint64_t) + MEMORY_PER_PROCESS;

    uint64_t *shm_ptr = mmap(0, shm_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm_ptr == MAP_FAILED) {
        perror("mmap (subprocess)");
        retcode = errno;
        goto exit;
    }
    shm_ptr += offset;
    char *shm_input = (char*)(shm_ptr + NUMBERS_FOR_COLLATZ + NUMBERS_FOR_METADATA);
    uint64_t collatz = checked_strtoull(shm_input);
    if(collatz == 0) {
        *shm_ptr = 0;
        *(shm_ptr + 1) = ERROR_PARSE_FAILED;
        retcode = EXIT_FAILURE;
        goto exit;
    }

    int remaining_space = NUMBERS_FOR_COLLATZ;
    for(;;) {
        if(remaining_space == 0) {
            *shm_ptr = 0;
            *(shm_ptr + 1) = ERROR_SHARED_BUFFER_OVERFLOW;
            retcode = EXIT_FAILURE;
            goto exit;
        }

        *shm_ptr = collatz;
        shm_ptr++;
        remaining_space--;

        if(collatz == 1) {
            *shm_ptr = 0;
            *(shm_ptr + 1) = ERROR_NONE;
            retcode = 0;
            goto exit;
        }

        if((collatz % 2ull) == 0) {
            collatz /= 2ull;
        } else {
            if((collatz >= (ULLONG_MAX / 3ull)) || (collatz * 3ull == ULLONG_MAX)) {
                *shm_ptr = 0;
                *(shm_ptr + 1) = ERROR_INTEGER_OVERFLOW;
                retcode = EXIT_FAILURE;
                goto exit;
            }
            collatz = collatz * 3 + 1;
        }
    }

    exit:
    printf("%s Parent %d Me %d\n", retcode == 0 ? "Done" : "Error", getppid(), getpid());
    if(shm_fd != -1) {
        close(shm_fd);
        if(shm_ptr != MAP_FAILED) {
            munmap(shm_ptr, shm_size);
        }
    }
    return retcode;
}

int main(int argc, char **argv, char **envp) {
    if(argc <= 1) {
        printf("Usage: shmcollatz <nr_1 nr_2 ... nr_n> | <%s (implementation detail) OFFSET\n", SUBPROCESS_FLAG);
        return EXIT_FAILURE;
    } else if(argc == 3 && (strcmp(argv[1], SUBPROCESS_FLAG) == 0)) {
        exit(main_subprocess(argv));
    } else {
        exit(main_mainprocess(argc, argv, envp));
    }
}
