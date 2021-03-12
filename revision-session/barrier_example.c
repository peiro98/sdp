/*
 * The process generates N threads. Each thread executes the function:
 *
 * void f () {
 *   A();
 *   B();
 * }
 *
 * Before executing B() each thread must wait until all the N threads
 * have executed A().
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N 10

void* f (void *arg);

sem_t mutex, barrier;

int main (int argc, char **argv) {
    pthread_t tids[N];
    int n = 0;

    srand(getpid());

    sem_init(&mutex, 0, 1);
    sem_init(&barrier, 0, 0);

    for (int i = 0; i < N; i++) {
        pthread_create(tids + i, NULL, &f, &n);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(tids[i], NULL);
    }
}

void* f (void *arg) {
    int* n = (int*) arg;

    printf("[Thread %ld]: executing A...\n", pthread_self());
    sleep(rand() % 5);

    // access to the critical section
    sem_wait(&mutex);
    if (++(*n) == N) {
        // all the threads completed A()
        for (int i = 0; i < N; i++) sem_post(&barrier);
    }
    sem_post(&mutex);

    // wait until all the threads completed A()
    sem_wait(&barrier);

    printf("[Thread %ld]: executing B...\n", pthread_self());
    sleep(rand() % 5);
}

