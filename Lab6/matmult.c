#include <semaphore.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#define i64 int64_t

sem_t threads_finished;
sem_t threads_running;
sem_t threads_running_or_idle;

void sem_init_noerr(sem_t *sem, int value) {
    if(sem_init(sem, 0, value) < 0) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
}

void sem_wait_noerr(sem_t *sem) {
    if(sem_wait(sem) < 0) {
        perror("sem_wait");
        exit(EXIT_FAILURE);
    }
}

void sem_post_noerr(sem_t *sem) {
    if(sem_post(sem) < 0) {
        perror("sem_post");
        exit(EXIT_FAILURE);
    }
}

const int RUNNING_THREADS_ALLOWED = 16;
const int RUNNING_OR_IDLE_THREADS_ALLOWED = RUNNING_THREADS_ALLOWED * 2;

bool multiply(i64 x, i64 y, i64 *out) {
    #if __GNUC__ | __clang__
        if(sizeof(i64) == sizeof(long long int)) {
            return __builtin_smull_overflow(x, y, out);
        }
    #endif
    if(y > INT64_MAX) {
        if(x > INT64_MAX / y || x < INT64_MAX / y) {
            return true;
        }
    } else if(y < 0) {
        if(y == -1 && x == INT64_MIN) {
            return true;
        }
        if(x < INT64_MAX / y || x > INT64_MAX / y) {
            return true;
        }
    }

    *out = x * y;
    return false;
}

typedef struct matrix {
    i64 *data;
    i64 rows;
    i64 columns;
} matrix_t;

// No need to ever free the data in these.
// Only 3 will ever be allocated.
// The OS will free them on exit.
matrix_t a, b, out;

#define MATELEM(MATRIX, ROW, COLUMN) ((MATRIX).data[ROW * ((MATRIX).rows) + COLUMN])

void alloc_mat(matrix_t *mat) {
    i64 alloc_size;
    if(multiply(mat->rows, mat->columns, &alloc_size) || multiply(alloc_size, sizeof(i64), &alloc_size)) {
        fprintf(stderr, "Overflow computing size of %"PRIi64"x%"PRIi64" matrix.\n", mat->rows, mat->columns);
        exit(EXIT_FAILURE);
    }
    mat->data = malloc(alloc_size);
    if(mat->data == NULL) {
        fprintf(stderr, "Could not allocate %"PRIi64"x%"PRIi64" matrix.\n", mat->rows, mat->columns);
    }
}

void read_matrix(matrix_t *m) {
    if(scanf(PRIi64 " " PRIi64, &m->rows, &m->columns) == EOF) {
        perror("scanf");
        exit(EXIT_FAILURE);
    }
    alloc_mat(m);
    for(i64 row = 0;row < m->rows;row++) {
        for(i64 col = 0;col < m->columns;col++) {
            if(scanf(PRIi64, &MATELEM(*m, row, col)) == EOF) {
                perror("scanf");
                exit(EXIT_FAILURE);
            }
        }
    }
}

typedef struct matmult_thread_args {
    i64 out_row;
    i64 out_col;
} matmult_args_t;

void * matmult_thread(void *args_void) {
    sem_wait(&threads_running);
    matmult_args_t *args = args_void;
    ///TODO implement multiply
    sem_post(&threads_running);
    sem_post(&threads_running_or_idle);
    sem_post(&threads_finished);
}

int main() {
    sem_init_noerr(&threads_finished, 0);
    sem_init_noerr(&threads_running, RUNNING_THREADS_ALLOWED);
    sem_init_noerr(&threads_running_or_idle, RUNNING_OR_IDLE_THREADS_ALLOWED);
    read_matrix(&a);
    read_matrix(&b);
    if(a.columns != b.rows) {
        fprintf(stderr, "Incompatible matrix sizes %"PRIi64"x%"PRIi64" %"PRIi64"x%"PRIi64"\n", a.rows, a.columns, b.rows, b.columns);
        return EXIT_FAILURE;
    }
    out.rows = a.rows;
    out.columns = b.columns;
    alloc_mat(&out);
    for(i64 row = 0;row < out.rows;row++) {
        for(i64 col = 0;col < out.columns;col++) {
            sem_wait_noerr(&threads_running_or_idle);
            pthread_t thr;
            matmult_args_t *args = malloc(sizeof(matmult_args_t));
            args->out_row = row;
            args->out_col = col;
            if(pthread_create(&thr, NULL, matmult_thread, args)) {
                perror("pthread_create");
                return errno;
            }
            if(pthread_detach(&thr)) {
                perror("pthread_detach");
                return errno;
            }
        }
    }
    for(i64 x = 0;x < out.rows * out.columns;x++) {
        sem_post(&threads_finished);
    }
    printf("%"PRIi64"x%"PRIi64" output matrix:\n", out.rows, out.columns);
    for(i64 row = 0;row < out.rows;row++) {
        for(i64 col = 0;col < out.columns;col++) {
            printf("%"PRIi64" ", MATELEM(out, row, col));
        }
        printf("\n");
    }
    return EXIT_SUCCESS;
}