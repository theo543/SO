#include <semaphore.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "macros.h"

#define i64 int64_t

const char* COUNT_FLAG = "--peak-threads";
bool counting_threads = false;
i64 _Atomic thread_counter = 0;

#define ASSERT_LONGLONG_IS_I64 static_assert(sizeof(long long) == sizeof(i64), "long long must equal i64")
#ifndef NO_OVERFLOW_CHECKS
    #define NO_OVERFLOW_CHECKS 0
#endif
#ifndef NO_BUILTIN_OVERFLOW_CHECKS
    #define NO_BUILTIN_OVERFLOW_CHECKS 0
#endif

bool multiply(i64 x, i64 y, i64 *out) {
    #if NO_OVERFLOW_CHECKS
        *out = x * y;
        return false;
    #elif (__GNUC__ || __clang__) && !NO_BUILTIN_OVERFLOW_CHECKS
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
    #if NO_OVERFLOW_CHECKS
        *out = x + y;
        return false;
    #elif (__GNUC__ || __clang__) && !NO_BUILTIN_OVERFLOW_CHECKS
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

    i64 peak_threads;

    pthread_t handle;
} matmult_args_t;

void * matmult_thread(void *args_void) {
    matmult_args_t *args = args_void;

    if(counting_threads) {
        i64 old = atomic_fetch_add(&thread_counter, 1);
        args->peak_threads = (old + 1);
    }

    i64 i = args->out_row;
    i64 j = args->out_col;
    i64 sum = 0;

    for(i64 k = 0;k < a.columns;k++) {
        i64 result;
        if(multiply(MATELEM(a, i, k), MATELEM(b, k, j), &result) || add(sum, result, &sum)) {
            fprintf(stderr, "Overflow calculating result at position %"PRIi64"x%"PRIi64"\n", i, j);
            exit(EXIT_FAILURE);
        }
    }

    MATELEM(out, i, j) = sum;

    if(counting_threads) {
        atomic_fetch_sub(&thread_counter, 1);
    }

    return NULL;
}

int main(int argc, char **argv) {
    for(int x = 1;x < argc;x++) {
        if(strcmp(argv[x], COUNT_FLAG) == 0) {
            counting_threads = true;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[x]);
            return EXIT_FAILURE;
        }
    }

    read_matrix(&a);
    read_matrix(&b);
    if(a.columns != b.rows) {
        fprintf(stderr, "Incompatible matrix sizes %"PRIi64"x%"PRIi64" %"PRIi64"x%"PRIi64"\n", a.rows, a.columns, b.rows, b.columns);
        return EXIT_FAILURE;
    }

    out.rows = a.rows;
    out.columns = b.columns;
    alloc_mat(&out);

    matmult_args_t *args_mem = calloc(out.rows * out.columns, sizeof(matmult_args_t));
    if(args_mem == NULL) {
        fprintf(stderr, "Could not allocate memory for thread arguments\n");
        return EXIT_FAILURE;
    }

    for(i64 row = 0;row < out.rows;row++) {
        for(i64 col = 0;col < out.columns;col++) {
            matmult_args_t *args = args_mem + (row * out.columns + col);
            args->out_row = row;
            args->out_col = col;
            PT_CALL(pthread_create(&args->handle, NULL, matmult_thread, args));
        }
    }

    for(i64 x = 0;x < out.rows * out.columns;x++) {
        PT_CALL(pthread_join(args_mem[x].handle, NULL));
    }

    if(counting_threads) {
        i64 peak_threads = 0;
        for(i64 i = 0;i < out.rows * out.columns;i++) {
            if(args_mem[i].peak_threads > peak_threads)
                peak_threads = args_mem[i].peak_threads;
        }
        fprintf(stderr, "Peak number of threads: %"PRIi64"\n", peak_threads);
    }

    free(args_mem);

    printf("%"PRIi64" %"PRIi64"\n", out.rows, out.columns);
    for(i64 row = 0;row < out.rows;row++) {
        for(i64 col = 0;col < out.columns;col++) {
            printf("%"PRIi64" ", MATELEM(out, row, col));
        }
        printf("\n");
    }

    free(a.data);
    free(b.data);
    free(out.data);

    return EXIT_SUCCESS;
}
