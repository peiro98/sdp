/*
Implement a *sequential* program in C that:
- Takes a single argument n from the command line
- Generates two array (v1 and v2) of size n, and a matrix (mat) of
  dimension (n x n), assigning to all elements random values.
  All random values must be in the range [-0.5, 0.5]
- Evaluates the following product
  res = v1^T * mat * v2
  (where v1^T is the transpose of v1).
  The computation must be performed in two steps, as the program must first
  compute
  v = v1^T * mat
  and then evaluate
  res = v * v2
- Prints the result res.

For example, let us suppose that n=5, and v1, v2 and mat are the
following ones:

v1^T = [ -0.0613 -0.1184  0.2655  0.2952 -0.3131 ] 

mat  = [ -0.3424 -0.3581  0.1557  0.2577  0.2060	
          0.4706 -0.0782 -0.4643  0.2431 -0.4682
          0.4572  0.4157  0.3491  0.1078 -0.2231	
         -0.0146  0.2922  0.4340 -0.1555 -0.4029
          0.3003  0.4595  0.1787 -0.3288 -0.4656 ]

v2^T = [ -0.3235  0.1948 -0.1829  0.4502 -0.4656 ]

Then, the result of the computation is:

res = v1^T * mat * v2 = (v1^T * mat) * v2 = -0.004680

After the sequential program correctly computes the final result,
transform it into two *concurrent* programs using Pthreads.

Organize the version A of the concurrent program as follows:

- The main thread creates the arrays, the matrix, and it runs n threads.
  Then, it waits the termination of all n threads.
- Each thread i performs the product of the array v1^T with i-th row
  of mat, which produces the i-th element of vector v.
- When all threads have terminated their task, the main thread compute
  the final result as
  res = v * v2

Organize the version B of the concurrent program as version A, but
once v has been computed, the final result (i.e., res = v * v2) is computed
by one of the created threads (not by the main threads).
Force the program to use the last thread which terminate the first part of the
task to compute the second part.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include "matrix.h"

typedef struct th_args_s
{
    pthread_t tid;
    mat_t *m1;
    mat_t *m2;
    int ncol;
    float *res; // result destination
} th_args_t;

long get_time_us();

static void _print_matrix(const char *name, mat_t *m);

static void *compute_product(void *arg);

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "USAGE: ./a.out n\n");
        return 1;
    }

    int n = atoi(argv[1]);
    th_args_t *th_args = malloc(n * sizeof(th_args_t));

    srand(get_time_us());

    mat_t *v1, *v2, *M;
    mat_t *v1_t, *v2_t;

    initialize_matrix(&v1, n, 1);
    initialize_matrix(&v2, n, 1);
    initialize_matrix(&v1_t, 1, n);
    initialize_matrix(&M, n, n);

    // fill the matrices with random values
    randomize(v1);
    randomize(v2);
    randomize(M);

    transpose(v1_t, v1);

    _print_matrix("v1", v1);
    _print_matrix("v1.T", v1_t);

    _print_matrix("v2", v2);

    _print_matrix("M", M);

    mat_t *tmp;
    initialize_matrix(&tmp, v1_t->nrows, M->ncols); // 1x5
    for (int i = 0; i < n; i++)
    {
        th_args[i].m1 = v1_t;
        th_args[i].m2 = M;
        th_args[i].ncol = i;
        th_args[i].res = tmp->data + i;
        pthread_create(&th_args[i].tid, NULL, &compute_product, (void *)&th_args[i]);
    }

    for (int i = 0; i < n; i++)
    {
        pthread_join(th_args[i].tid, NULL);
    }

    mat_t *res;
    initialize_matrix(&res, tmp->nrows, v2->ncols);

    dot(res, tmp, v2);
    _print_matrix("res", res);

    free_matrix(v1);
    free_matrix(v1_t);
    free_matrix(v2);
    free_matrix(M);
    free_matrix(tmp);
    free_matrix(res);
}

long get_time_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 1000 * 1000 * tv.tv_sec + tv.tv_usec;
}

static void _print_matrix(const char *name, mat_t *m)
{
    printf("%s:\n", name);
    print_matrix(m);
    printf("\n\n");
}

static void *compute_product(void *arg)
{
    th_args_t *th_args = (th_args_t *)arg;
    *(th_args->res) = row_column_product(th_args->m1, 0, th_args->m2, th_args->ncol);
    pthread_exit(NULL);
}