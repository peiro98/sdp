/*
Given an array v of 2^n elements v[i], i.e.,

v = { v[0], v[1], ... , x[2^n − 1] }

its k-th prefix sum (the cumulative sum vector) is the sum of its first k+1
elements, i.e.,
v[0] + v[1] + ... + v[k]

Write a C program using Pthreads that:
* Receives an integer value n as argument of the command line
* Randomly generates an array v of 2^n integer elements in the range [1-9]
* Computes the prefix sum of v as follows.

For an array v of 2^n elements, it is possible to compute all values of
the prefix sum in n steps, by means of 2^n−1 concurrent threads, each
associated to an array element.

For example for n=3 and 2^3 = 8 elements, each step consists of each
thread taking two elements, and computing a result:
* In the first step each thread computes the sums of adjacent elements
  (i.e., gap = 1=2^0). 
* In the second step, each thread computes the sums of the elements
  that are 2 elements away (i.e., gap = 2 = 2^1).
* In the third step, each thread computes the sums of the elements
  that are 4 elements away (i.e., gap = 4 = 2^2).
After 3 steps, all sum prefix values are computed.

In general, if we have 2^n elements in v, step i computes the sums of
elements that are 2^(i-1) elements away.
After n steps, all cumulative sum values are computed.

For example, let us suppose the program is run as:

> cumulative_sum  3

It should compute (and eventually display) the following:

Initial array v:  2  4  6  1  3  5  8  7
v after step 1 :  2  6 10  7  4  8 13 15
                  Computed as: v[0]=v[0], v[1]=v[0]+v[1],
		               v[2]=v[1]+v[2], etc., v[n-1]=v[n-2]+v[n-1]
v after step 2 :  2  6 12 13 14 15 17 23
                  Computed as: v[0]=v[0], v[1]=v[1], v[2]=v[0]+v[2],
                               v[3]=v[1]+v[3], etc., v[n-1]=v[n-3]+v[n-1]
v after step 3 :  2  6 12 13 16 21 29 36
                  Computed as: v[0]=v[0], etc., v[3]=v[4], v[4]=v[0]+v[4],
                               v[5]=v[1]+v[5], etc., v[n-1]=v[n-5]+v[n-1]

Please notice that:

1. The main thread allocates, fills, and prints the initial content of
   the array v

2. All threads are initially created by the main thread, they have all
   the same code, and they must synchronize their activity according to the
   procedure previously described, exploiting the maximum possibile
   concurrency.

3. The array v must be updated in place, i.e., you cannot allocate
   auxiliary arrays.

4. Each thread must loop for no more than n times (the number of
   steps) before exiting.

5. Some threads terminate before the n-th step.
   In particular thread i terminates after step i, with i in range [1–n].

6. The main thread prints immediately the value in x[0], then it waits
   the termination of each threads i, in order of creation, and
   immediately prints the value stored in x[i].


Suggestions
-----------
* First design and write the pseudo-code of a solution excluding rule 5.
* Then, update your solution including rule 5.
  Please notice that:
  - If you refer to x[i-gap], index i-gap must be non negative
  - You must properly manage a variable that stores the number of
    active threads, which must be decremented according to rule 5.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

void fill_array(int *arr, int size);
void print_array(int *arr, int size);
long get_time_us();

static void *th_foo(void *arg);

typedef struct th_arg_s
{
    int *array; // pointer to the array
    pthread_t tid;
    int n; // number of iterations
    int i; // assigned position

    pthread_mutex_t *lock;
    pthread_cond_t *cond;

    int *pending; // number of threads waiting at a synchronization point
    int *active;  // number of active threads
} th_arg_t;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "USAGE: ./a.out\n");
        return 1;
    }

    srand(get_time_us());

    int n = atoi(argv[1]);

    int size = 1 << n; // 2^n
    int *arr = (int *)malloc(size * sizeof(int));

    fill_array(arr, size);
    print_array(arr, size);

    th_arg_t *th_args = (th_arg_t *)malloc(size * sizeof(th_arg_t));

    // initialize the lock and the condition variable
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

    int active = size;
    int pending = 0;

    for (int i = 0; i < size; i++)
    {
        th_args[i].lock = &lock;
        th_args[i].cond = &cond;

        th_args[i].array = arr;
        th_args[i].n = n;
        th_args[i].i = i;
        th_args[i].active = &active;
        th_args[i].pending = &pending;
        pthread_create(&th_args[i].tid, NULL, &th_foo, (void *)&th_args[i]);
    }

    printf("%2d", arr[0]);
    for (int i = 1; i < size; i++)
    {
        // wait the i-th thread
        pthread_join(th_args[i].tid, NULL);
        printf(" %2d", arr[i]);
    }
    printf("\n");

    free(th_args);
}

void fill_array(int *arr, int size)
{
    for (int i = 0; i < size; i++)
    {
        arr[i] = rand() % 9 + 1;
    }
}

long get_time_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 1000 * 1000 * tv.tv_sec + tv.tv_usec;
}

void print_array(int *arr, int size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%2d ", arr[i]);
    }

    printf("\n");
}

static void *th_foo(void *arg)
{
    th_arg_t *th_arg = (th_arg_t *)arg;

    int gap = 1;
    int loop = 1;
    for (int i = 0; i < th_arg->n && loop; i++)
    {
        // read the value in position i
        int s = th_arg->array[th_arg->i];

        // read the value in position (i - gap)
        if (th_arg->i - gap >= 0)
        {
            s += th_arg->array[th_arg->i - gap];
        }
        else
        {
            loop = 0;
        }

        // access to the critical section
        pthread_mutex_lock(th_arg->lock);
        *(th_arg->pending) += 1; // increment the number of pending threads

        if (*(th_arg->pending) == *(th_arg->active))
        {
            // all the threads completed the read operation
            // reset the number of pending threads and wake up all the waiting threads
            *(th_arg->pending) = 0;
            pthread_cond_broadcast(th_arg->cond);
        }
        else
        {
            // wait
            pthread_cond_wait(th_arg->cond, th_arg->lock);
        }
        pthread_mutex_unlock(th_arg->lock);

        // write the result in position i
        th_arg->array[th_arg->i] = s;

        // access to the critical section
        pthread_mutex_lock(th_arg->lock);
        *(th_arg->pending) += 1; // increment the number of pending threads

        if (*(th_arg->pending) == *(th_arg->active))
        {
            *(th_arg->pending) = 0;
            pthread_cond_broadcast(th_arg->cond);
        }
        else
        {
            pthread_cond_wait(th_arg->cond, th_arg->lock);
        }

        // if the current threads terminates before n iterations, reduce the number of active threads
        *(th_arg->active) -= (loop == 0) ? 1 : 0;

        pthread_mutex_unlock(th_arg->lock);

        gap <<= 1;
    }
}