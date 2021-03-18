/*
Use a conditional variable to implement the following behavior:

A program creates 3 threads
    - Thread 1 and thread 2
        - Update a variable (count), increasing it at each iteration
    - Thread 3
        - Awaits until the variable (count) reaches a specified value
        - When the value is reached, it displays a message on standard output
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define N_THREADS 3
#define LIMIT 5

#define INT_MIN(a, b) ((a < b) ? a : b)

typedef struct cond_s
{
    pthread_mutex_t mutex;
    pthread_cond_t unlock_condition;
    int counter;
} cond_t;

void *counter(void *arg)
{
    int loop = 1;
    cond_t *my_condition = (cond_t *)arg;

    while (loop)
    {
        pthread_mutex_lock(&my_condition->mutex);
        int old_value = my_condition->counter;
        my_condition->counter = INT_MIN(old_value + 1, LIMIT);

        if (my_condition->counter == LIMIT)
        {
            if (old_value != LIMIT)
            {
                // Note: The pthread_cond_broadcast() and pthread_cond_signal() functions 
                // shall have no effect if there are no threads currently blocked on cond.
                pthread_cond_signal(&my_condition->unlock_condition);
            }
            loop = 0;
        }

        if (old_value != my_condition->counter)
        {
            printf("[%ld]: current value of the counter %d\n", (long)pthread_self(), my_condition->counter);
        }

        pthread_mutex_unlock(&my_condition->mutex);
    }

    pthread_exit(NULL);
}

void *watcher(void *arg)
{
    cond_t *my_condition = (cond_t *)arg;

    pthread_mutex_lock(&my_condition->mutex);

    printf("[%ld]: watcher started...\n", pthread_self());

    // check the current value of the counter
    if (my_condition->counter < LIMIT)
    {
        // if the counter is below the limit, wait on the condition variable
        pthread_cond_wait(&my_condition->unlock_condition, &my_condition->mutex);
    }

    printf("[%ld]: watcher completed!\n", pthread_self());
    pthread_mutex_unlock(&my_condition->mutex);
}

int main(int argc, char **argv)
{
    // threads
    pthread_t tids[N_THREADS];

    // condition initialization
    cond_t my_condition = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0};

    // initialize the counter
    for (int i = 0; i < N_THREADS - 1; i++)
    {
        pthread_create(&tids[i], NULL, &counter, (void *)&my_condition);
    }

    // initialize the watcher
    pthread_create(&tids[N_THREADS - 1], NULL, &watcher, (void *)&my_condition);

    // wait for all the threads
    for (int i = 0; i < N_THREADS; i++)
    {
        pthread_join(tids[i], (void **)NULL);
    }

    exit(EXIT_SUCCESS);
}