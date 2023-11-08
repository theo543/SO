// C stdlib
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
// Linux
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>

const char *SHM_NAME = "/SHMCOLLATZ_SHARED_MEMORY_SEGMENT";
const char *SUBPROCESS_FLAG = "-shm";
const int MAX_NUMBERS_ALLOWED = 3000 + 2; // 3000 collatz numbers, plus 2 for reporting the result

int calculate_mem_per_process() {
    int page_size = getpagesize();
    int needed_memory = MAX_NUMBERS_ALLOWED * sizeof(uint64_t);
    int pages = ((needed_memory + page_size - 1) / page_size);
    return pages * page_size;
}

// strtoull but only reading valid positive integers, prints an warning to stderr and returns 0 for errors
uint64_t checked_strtoull(char *input) {
    char *parse_result;
    errno = 0;
    uint64_t collatz = strtoull(input, &parse_result, 10);
    if(*parse_result != '\0' || errno == ERANGE || collatz == 0 || strchr(input, '-')) {
        fprintf(stderr, "Failed to parse input '%s' as positive unsigned long long\n", input);
        return 0;
    }
    return collatz;
}

int main_mainprocess(int argc, char **argv, char **envp) {
    printf("Starting parent %d", getpid());

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
        char offset_str[sizeof(uint64_t) + 1];
        offset_str[sizeof(uint64_t)] = 0;
        uint64_t subprocess_offset = mem_per_proc * (x - 1);
        // convert uint64_t to bytes using memcpy
        memcpy(offset_str, &subprocess_offset, sizeof(uint64_t));

        // args to pass to subprocess (null-terminated)
        char *new_argv[3] = {offset_str, argv[x], NULL};

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

    for(int x = 0;x<argc;x++) {
        printf("%s:", argv[x]);
        uint64_t *shm_iter = shm_ptr + mem_per_proc * (x - 1);
        while(*shm_iter != 0) {
            printf(" %ld", *shm_iter);
        }
        //TODO: report errors from subprocess via second value
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
    // TODO: connect to shared memory, receive offset and number, process one number

    printf("Done Parent %d Me %d", getppid(), getpid());
    return EXIT_SUCCESS;
}

int main(int argc, char **argv, char **envp) {
    if(argc <= 1) {
        printf("Usage: shmcollatz <nr_1 nr_2 ... nr_n> | <%s (implementation detail) OFFSET NUMBER\n", SUBPROCESS_FLAG);
        return EXIT_FAILURE;
    } else if(argc == 2 && strcmp(argv[1], SUBPROCESS_FLAG)) {
        exit(main_subprocess(argc, argv, envp));
    } else {
        exit(main_mainprocess(argc, argv, envp));
    }
}
