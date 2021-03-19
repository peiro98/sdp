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
  mat_t *v1_t;
  mat_t *M;
  mat_t *v2;
  int i;
  mat_t *tmp;
  pthread_mutex_t lock;
  int *pending;
  mat_t *res;
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

  mat_t *v1, *v1_t, *v2, *M;

  initialize_matrix(&v1, n, 1);   // n x 1
  initialize_matrix(&v2, n, 1);   // n x 1
  initialize_matrix(&v1_t, 1, n); // 1 x n
  initialize_matrix(&M, n, n);    // n x n

  // fill the matrices with random values
  randomize(v1);
  randomize(v2);
  randomize(M);

  transpose(v1_t, v1);

  _print_matrix("v1", v1);
  _print_matrix("v1.T", v1_t);

  _print_matrix("v2", v2);

  _print_matrix("M", M);

  mat_t *tmp, *res;
  initialize_matrix(&tmp, v1_t->nrows, M->ncols); // 1x5
  initialize_matrix(&res, tmp->nrows, v2->ncols); // 1x1

  int pending = n;
  pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
  for (int i = 0; i < n; i++)
  {
    th_args[i].v1_t = v1_t;
    th_args[i].M = M;
    th_args[i].v2 = v2;
    th_args[i].i = i;
    th_args[i].tmp = tmp;
    // lock used to decrement and check the 'pending' counter
    th_args[i].lock = lock;
    th_args[i].pending = &pending;
    th_args[i].res = res;
    pthread_create(&th_args[i].tid, NULL, &compute_product, (void *)&th_args[i]);
  }

  for (int i = 0; i < n; i++)
  {
    pthread_join(th_args[i].tid, NULL);
  }

  _print_matrix("res", res);

  // free everything
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
  // compute the product of the row vector with the i-th column of the 
  // matrix M
  th_args->tmp->data[th_args->i] = row_column_product(th_args->v1_t, 0, th_args->M, th_args->i);

  // access to the critical section (decrement and check on pending)
  pthread_mutex_lock(&th_args->lock);
  *(th_args->pending) -= 1;
  // check if there are pending threads
  if (*(th_args->pending) == 0)
  {
    // if not, compute the last product
    dot(th_args->res, th_args->tmp, th_args->v2);
  }
  pthread_mutex_unlock(&th_args->lock);

  pthread_exit(NULL);
}