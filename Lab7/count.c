#include <signal.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "macros.h"

bool _Atomic threads_keep_running = true;

void ctrl_c_set_flag(int signum) {
    (void)signum;
    threads_keep_running = false;
}

const int THREADS = 2;
const int MAX_RESOURCES = 5;
int available_resources = MAX_RESOURCES;

pthread_mutex_t resource_mutex;

void lock(pthread_mutex_t *lock) {
    PT_CALL(pthread_mutex_lock(lock));
}

void unlock(pthread_mutex_t *lock) {
    PT_CALL(pthread_mutex_unlock(lock));
}

int decrease_count(int count) {
    lock(&resource_mutex);
    int ret;
    if(available_resources < count) {
        ret = -available_resources - 1;
    } else {
        available_resources -= count;
        ret = available_resources;
    }
    unlock(&resource_mutex);
    return ret;
}

int increase_count(int count) {
    lock(&resource_mutex);
    available_resources += count;
    int ret = available_resources;
    unlock(&resource_mutex);
    return ret;
}

void *resource_thread(void *rng_void) {
    unsigned int *rng_state = rng_void;
    while(threads_keep_running) {
        int amount = (rand_r(rng_state) % MAX_RESOURCES) + 1;
        int remain = decrease_count(amount);
        if(remain < 0) {
            printf("Couldn't get %d resources %d remaining\n", amount, -(remain + 1));
        } else {
            printf("Got %d resources %d remaining\n", amount, remain);
            remain = increase_count(amount);
            printf("Released %d resources %d remaining\n", amount, remain);
        }
    }
    printf("Exit\n");
    return NULL;
}

int main() {
    signal(SIGINT, ctrl_c_set_flag);
    PT_CALL(pthread_mutex_init(&resource_mutex, NULL));

    unsigned int *thread_rng_states = malloc(THREADS * sizeof(unsigned int));
    pthread_t *thread_handles = malloc(THREADS * sizeof(pthread_t));
    for(int x = 0;x < THREADS;x++) {
        thread_rng_states[x] = rand();
        PT_CALL(pthread_create(&thread_handles[x], NULL, resource_thread, &thread_rng_states[x]));
    }

    for(int x = 0;x < THREADS;x++) {
        PT_CALL(pthread_join(thread_handles[x], NULL));
    }

    free(thread_rng_states);
    free(thread_handles);
    PT_CALL(pthread_mutex_destroy(&resource_mutex));
}
