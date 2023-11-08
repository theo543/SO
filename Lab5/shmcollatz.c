// C stdlib
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
// Linux
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>

const char *SHM_NAME = "/SHMCOLLATZ_SHARED_MEMORY_SEGMENT";
const char *SUBPROCESS_FLAG = "-shm";
const int NUMBERS_FOR_METADATA = 2;
const int NUMBERS_FOR_COLLATZ = 3000;
const int MAX_NUMBERS_ALLOWED = NUMBERS_FOR_COLLATZ + NUMBERS_FOR_METADATA;

// used by subprocess to report errors
const uint64_t ERROR_NONE = 0;
const uint64_t ERROR_PARSE_FAILED = 1;
const uint64_t ERROR_INTEGER_OVERFLOW = 2;
const uint64_t ERROR_SHARED_BUFFER_OVERFLOW = 3;
const char *ERROR_MSG[] = {
    "No error. (should never be printed)",
    "Failed to parse input as positive integer.",
    "Integer overflow.",
    "Out of space to store results."
};

int calculate_mem_per_process() {
    int page_size = getpagesize();
    int needed_memory = MAX_NUMBERS_ALLOWED * sizeof(uint64_t);
    int pages = ((needed_memory + page_size - 1) / page_size);
    return pages * page_size;
}

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
    int mem_per_proc = calculate_mem_per_process();
    int shm_size = procs * mem_per_proc;

    int shm_fd = shm_open(SHM_NAME, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
    if(shm_fd < 0) {
        perror("shm_open");
        goto err;
    }
    if(ftruncate(shm_fd, shm_size) == -1) {
        perror("ftruncate");
        goto err;
    }
    uint64_t *shm_ptr = mmap(0, shm_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm_ptr == MAP_FAILED) {
        perror("mmap");
        goto err;
    }
    memset(shm_ptr, 0, shm_size);

    int self_location_fd = open("/proc/self/exe", O_RDONLY);
    if(self_location_fd == -1) {
        perror("open(\"/proc/self/exe\")");
        goto err;
    }

    for(int x = 1;x<argc;x++) {
        // send offset to subprocess via argument
        char offset_str[50];
        uint64_t subprocess_offset = mem_per_proc * (x - 1);
        sprintf(offset_str, "%ld", subprocess_offset);

        // args to pass to subprocess (null-terminated)
        char *new_argv[4] = {argv[0], offset_str, argv[x], NULL};

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
        uint64_t *shm_iter = shm_ptr + mem_per_proc * (x - 1);
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

    shm_unlink(SHM_NAME);
    return EXIT_SUCCESS;

    err:
    if(shm_fd >= 0) {
        shm_unlink(SHM_NAME);
    }
    return errno;
}

int main_subprocess(int argc, char **argv, char **envp) {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, S_IRUSR|S_IWUSR);
    if(shm_fd < 0) {
        perror("shm_open (subprocess)");
        return errno;
    }

    int mem_per_process = calculate_mem_per_process();
    // get offset
    uint64_t offset = strtoull(argv[1], NULL, 10);
    // not really the full size, but we don't need that
    int shm_size = offset + mem_per_process;

    uint64_t *shm_ptr = mmap(0, shm_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm_ptr == MAP_FAILED) {
        perror("mmap (subprocess)");
        return errno;
    }
    uint64_t *this_process_shm = shm_ptr + offset;

    uint64_t collatz = checked_strtoull(argv[2]);
    if(collatz == 0) {
        *this_process_shm = 0;
        this_process_shm++;
        *this_process_shm = ERROR_PARSE_FAILED;
        return EXIT_FAILURE;
    }

    int remaining_space = NUMBERS_FOR_COLLATZ;
    for(;;) {
        if(remaining_space == 0) {
            *this_process_shm = 0;
            this_process_shm++;
            *this_process_shm = ERROR_SHARED_BUFFER_OVERFLOW;
            return EXIT_FAILURE;
        }

        *this_process_shm = collatz;
        this_process_shm++;
        remaining_space--;

        if(collatz == 1) {
            *this_process_shm = 0;
            this_process_shm++;
            *this_process_shm = ERROR_NONE;
            break;
        }

        if((collatz % 2ull) == 0) {
            collatz /= 2ull;
        } else {
            if((collatz >= (ULLONG_MAX / 3ull)) || (collatz * 3ull == ULLONG_MAX)) {
                *this_process_shm = 0;
                this_process_shm++;
                *this_process_shm = ERROR_INTEGER_OVERFLOW;
                return EXIT_FAILURE;
            }
            collatz = collatz * 3 + 1;
        }
    }

    printf("Done Parent %d Me %d\n", getppid(), getpid());
    return EXIT_SUCCESS;
}

int main(int argc, char **argv, char **envp) {
    if(argc <= 1) {
        printf("Usage: shmcollatz <nr_1 nr_2 ... nr_n> | <%s (implementation detail) OFFSET NUMBER\n", SUBPROCESS_FLAG);
        return EXIT_FAILURE;
    } else if(argc == 3 && (strcmp(argv[1], SUBPROCESS_FLAG) == 0)) {
        exit(main_subprocess(argc, argv, envp));
    } else {
        exit(main_mainprocess(argc, argv, envp));
    }
}
