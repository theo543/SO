#include <semaphore.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#define i64 int64_t

sem_t threads_finished;
sem_t report_overflow_and_exit;

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

#define ASSERT_LONGLONG_IS_I64 static_assert(sizeof(long long) == sizeof(i64), "long long must equal i64")
#ifndef NO_BUILDIN_OVERFLOW_CHECKS
    #define NO_BUILTIN_OVERFLOW_CHECKS 0
#endif

bool multiply(i64 x, i64 y, i64 *out) {
    #if (__GNUC__ || __clang__) && !NO_BUILTIN_OVERFLOW_CHECKS
        ASSERT_LONGLONG_IS_I64;
        return __builtin_smulll_overflow(x, y, (long long*)out);
    #else
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
    #endif
}
bool add(i64 x, i64 y, i64 *out) {
    #if (__GNUC__ || __clang__) && !NO_BUILTIN_OVERFLOW_CHECKS
        ASSERT_LONGLONG_IS_I64;
        return __builtin_saddll_overflow(x, y, (long long*)out);
    #else
        if(y > 0 && x > INT64_MAX - y) {
            return true;
        } else if(y < 0 && x < INT64_MIN - y) {
            return true;
        }
        *out = x + y;
        return false;
    #endif
}
typedef struct matrix {
    i64 *data;
    i64 rows;
    i64 columns;
} matrix_t;

matrix_t a, b, out;

#define MATELEM(MATRIX, ROW, COLUMN) ((MATRIX).data[ROW * ((MATRIX).columns) + COLUMN])

void free_mats_atexit() {
    // These don't actually need to be freed, since they live until the end of the program,
    // the OS would free them. But that causes noise when using valgrind.
    if(a.data) free(a.data);
    if(b.data) free(b.data);
    if(out.data) free(out.data);
}

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
    if(scanf("%"PRIi64"%"PRIi64, &m->rows, &m->columns) != 2) {
        fprintf(stderr, "Invalid matrix size.");
        exit(EXIT_FAILURE);
    }
    alloc_mat(m);
    for(i64 row = 0;row < m->rows;row++) {
        for(i64 col = 0;col < m->columns;col++) {
            if(scanf("%"PRIi64, &MATELEM(*m, row, col)) != 1) {
                fprintf(stderr, "Invalid matrix element at position (%"PRIi64", %"PRIi64")\n", row, col);
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
    matmult_args_t *args = args_void;
    MATELEM(out, args->out_row, args->out_col) = args->out_row * out.columns + args->out_col;
    i64 i = args->out_row;
    i64 j = args->out_col;
    i64 sum = 0;
    for(i64 k = 0;k < a.columns;k++) {
        i64 result;
        if(multiply(MATELEM(a, i, k), MATELEM(b, k, j), &result) || add(sum, result, &sum)) {
            sem_wait_noerr(&report_overflow_and_exit);
            fprintf(stderr, "Overflow calculating result at position %"PRIi64"x%"PRIi64"\n", i, j);
            exit(EXIT_FAILURE);
        }
    }
    MATELEM(out, i, j) = sum;
    sem_post_noerr(&threads_finished);
    return NULL;
}

int main() {
    atexit(free_mats_atexit);
    sem_init_noerr(&threads_finished, 0);
    sem_init_noerr(&report_overflow_and_exit, 1);
    read_matrix(&a);
    read_matrix(&b);
    if(a.columns != b.rows) {
        fprintf(stderr, "Incompatible matrix sizes %"PRIi64"x%"PRIi64" %"PRIi64"x%"PRIi64"\n", a.rows, a.columns, b.rows, b.columns);
        return EXIT_FAILURE;
    }
    out.rows = a.rows;
    out.columns = b.columns;
    alloc_mat(&out);
    pthread_attr_t detached;
    if(pthread_attr_init(&detached) != 0) {
        perror("pthread_attr_init");
        return errno;
    }
    if(pthread_attr_setdetachstate(&detached, PTHREAD_CREATE_DETACHED) != 0) {
        perror("pthread_attr_setdetachstate");
        return errno;
    }
    matmult_args_t *args_mem = malloc(out.rows * out.columns * sizeof(matmult_args_t));
    for(i64 row = 0;row < out.rows;row++) {
        for(i64 col = 0;col < out.columns;col++) {
            pthread_t thr;
            matmult_args_t *args = args_mem + (row * out.columns + col);
            args->out_row = row;
            args->out_col = col;
            if(pthread_create(&thr, &detached, matmult_thread, args)) {
                perror("pthread_create");
                return errno;
            }
        }
    }
    pthread_attr_destroy(&detached);
    for(i64 x = 0;x < out.rows * out.columns;x++) {
        sem_wait_noerr(&threads_finished);
    }
    free(args_mem);
    printf("%"PRIi64" %"PRIi64"\n", out.rows, out.columns);
    for(i64 row = 0;row < out.rows;row++) {
        for(i64 col = 0;col < out.columns;col++) {
            printf("%"PRIi64" ", MATELEM(out, row, col));
        }
        printf("\n");
    }
    return EXIT_SUCCESS;
}
