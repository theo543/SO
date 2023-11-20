#include <semaphore.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
// Linux
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "macros.h"

pid_t gettid(void) {
    return syscall(__NR_gettid);
}

typedef struct thread_barrier {
    sem_t entry_semaphore;
    sem_t waiting_semaphore;
    int blocked_threads;
    int max_threads;
} barrier_t;

barrier_t barrier;

void barrier_init(barrier_t *barrier, int threads) {
    SEM_CALL(sem_init(&barrier->entry_semaphore, 0, 1));
    SEM_CALL(sem_init(&barrier->waiting_semaphore, 0, 0));
    barrier->blocked_threads = 0;
    barrier->max_threads = threads;
}

void barrier_destroy(barrier_t *barrier) {
    SEM_CALL(sem_destroy(&barrier->entry_semaphore));
    SEM_CALL(sem_destroy(&barrier->waiting_semaphore));
}

void barrier_point(barrier_t *barrier) {
    if(barrier->max_threads == 1) {
        return;
    }
    SEM_CALL(sem_wait(&barrier->entry_semaphore));
    if((barrier->blocked_threads + 1) == barrier->max_threads) {
        SEM_CALL(sem_post(&barrier->waiting_semaphore));
        // do not unlock the semaphore
        // pass it on to the thread which wakes up now
        return;
    }
    barrier->blocked_threads += 1;
    SEM_CALL(sem_post(&barrier->entry_semaphore));
    SEM_CALL(sem_wait(&barrier->waiting_semaphore));
    // now we have the entry_semaphore again
    barrier->blocked_threads--;
    if(barrier->blocked_threads > 0) {
        // pass it on again
        SEM_CALL(sem_post(&barrier->waiting_semaphore));
    } else {
        // finally unlock it so threads can start queuing up again
        SEM_CALL(sem_post(&barrier->entry_semaphore));
    }
}

void * barrier_thread(void *num_repeats_void) {
    static_assert(sizeof(long long) >= sizeof(pid_t), "pid_t must fit in a long long so that it can be printf-ed");
    int num_repeats = *(int*)num_repeats_void;
    long long tid = gettid();
    for(int i = 0;i < num_repeats;i++) {
        printf ("%lld reached the barrier\n", tid);
        barrier_point(&barrier);
        printf ("%lld passed the barrier\n", tid);
    }
    return NULL;
}

int main (int argc, char **argv) {
    if(argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: barrier <THREADS> [<REPEATS>]\n");
        return EXIT_FAILURE;
    }
    int threads = atoi(argv[1]);
    if(threads == 0) {
        fprintf(stderr, "Invalid number of threads.\n");
        return EXIT_FAILURE;
    }
    int num_repeats = 1;
    if(argc == 3) {
        num_repeats = atoi(argv[2]);
        if(num_repeats == 0) {
            fprintf(stderr, "Invalid number of repeats.\n");
            return EXIT_FAILURE;
        }
    }

    barrier_init(&barrier, threads);

    pthread_t *handles = malloc(threads * sizeof(pthread_t));
    for(int x = 0;x < threads;x++) {
        PT_CALL(pthread_create(&handles[x], NULL, barrier_thread, &num_repeats));
    }
    for(int x = 0;x < threads;x++) {
        PT_CALL(pthread_join(handles[x], NULL));
    }
    free(handles);
}
