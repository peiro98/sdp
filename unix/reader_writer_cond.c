/*
Implement the First Reader-Writer (with the reader precedence) scheme using
    - Mutexes
    - --> Condition Variables
    - Read-Write Locks
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define CONCURRENCY_LVL 20

typedef struct rw_s
{
    int nr;
    int nw;
    long zero_us;
    pthread_mutex_t lock;
    pthread_cond_t turn;
} rw_t;

long get_time_us();
void *reader(void *arg);
void *writer(void *arg);
void wait_random_ns(long max);

rw_t rw = {0, 0, 0L, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};

int main(int argc, char **argv)
{
    pthread_t readers[CONCURRENCY_LVL];
    pthread_t writers[CONCURRENCY_LVL];

    rw.zero_us = get_time_us();

    srand(getpid());

    setbuf(stdout, NULL);

    for (long int i = 0; i < CONCURRENCY_LVL; i++)
    {
        pthread_create(&readers[i], NULL, &reader, (void *)(i));
        pthread_create(&writers[i], NULL, &writer, (void *)(CONCURRENCY_LVL + i));

        // detach the threads: no need to join them
        pthread_detach(readers[i]);
        pthread_detach(writers[i]);
    }

    pthread_exit(NULL);
}

long get_time_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 1000 * 1000 * tv.tv_sec + tv.tv_usec;
}

void *reader(void *arg)
{
    int i = (long int)arg;

    wait_random_ns(1000 * 1000 * 1000);

    printf("[+%8ld] reader %2d: trying to read...\n", get_time_us() - rw.zero_us, i);

    // wait for the 'lock' mutex
    pthread_mutex_lock(&rw.lock);
    while (rw.nw > 0) // wait as long as there are writers waiting
        pthread_cond_wait(&rw.turn, &rw.lock);
    rw.nr++;
    pthread_mutex_unlock(&rw.lock);

    printf("[+%8ld] reader %2d: reading...\n", get_time_us() - rw.zero_us, i);
    wait_random_ns(1000 * 1000 * 1000);

    pthread_mutex_lock(&rw.lock);
    rw.nr--;
    if (rw.nr == 0) // once all the readers completed, allow writers to proceed
        pthread_cond_broadcast(&rw.turn);
    pthread_mutex_unlock(&rw.lock);
}

void *writer(void *arg)
{
    int i = (long int)arg;

    wait_random_ns(500 * 1000 * 1000);

    printf("[+%8ld] writer %2d: trying to write...\n", get_time_us() - rw.zero_us, i);

    pthread_mutex_lock(&rw.lock);
    // wait as long as there are pending reads or writes
    while (rw.nr > 0 || rw.nw > 0)
    {
        pthread_cond_wait(&rw.turn, &rw.lock);
    }
    rw.nw++;
    pthread_mutex_unlock(&rw.lock);

    printf("[+%8ld] writer %2d: writing...\n", get_time_us() - rw.zero_us, i);
    wait_random_ns(500 * 1000 * 1000);

    pthread_mutex_lock(&rw.lock);
    rw.nw--;
    pthread_cond_broadcast(&rw.turn);
    pthread_mutex_unlock(&rw.lock);
}

void wait_random_ns(long max)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = rand() % max;
    nanosleep(&ts, NULL);
}
