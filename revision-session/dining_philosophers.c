#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

#define NUM_PHILOSOPHERS 10
#define RIGHT(i) ((i + 1) % NUM_PHILOSOPHERS)
#define LEFT(i) ((NUM_PHILOSOPHERS + i - 1) % NUM_PHILOSOPHERS)

typedef enum
{
    HUNGRY,
    EATING,
    THINKING
} state_t;

typedef struct
{
    pthread_t thread;
    state_t state;
    sem_t sem;
} philosopher_t;

sem_t mutex;
philosopher_t philosophers[NUM_PHILOSOPHERS];

void *start_philosophers(void *arg);
void take_forks(int id);
void put_forks(int id);
void test(int id);

int main(int argc, char **argv)
{
    sem_init(&mutex, 0, 1);

    srand(getpid());

    for (long int i = 0; i < NUM_PHILOSOPHERS; i++)
    {
        philosophers[i].state = THINKING;
        sem_init(&(philosophers[i].sem), 0, 0);
        pthread_create(&philosophers[i].thread, NULL, &start_philosophers, (void *)i);
    }

    pause();
}

void *start_philosophers(void *arg)
{
    int id = (long int)arg;

    while (1)
    {
        fprintf(stdout, "[Philosophers %d]: thinking...\n", id);
        sleep((rand() % 10) + 1);
        take_forks(id);
        fprintf(stdout, "[Philosophers %d]: eating...\n", id);
        sleep((rand() % 10) + 1);
        put_forks(id);
    }
}

void take_forks(int id)
{
    // wait for the access mutex
    // only one philosophers at the time can access the following section
    sem_wait(&mutex);
    // change the philosopher state to HUNGRY
    philosophers[id].state = HUNGRY;
    // check if the philosopher can eat
    test(id);
    sem_post(&mutex);
    // wait until the philosopher can eat
    sem_wait(&philosophers[id].sem);
}

void put_forks(int id)
{
    // wait for the access mutex
    // only one philosophers at the time can access the following section
    sem_wait(&mutex);
    philosophers[id].state = THINKING;
    // let the neighbours eat
    test(LEFT(id));
    test(RIGHT(id));
    sem_post(&mutex);
}

/*
 * Check if n-th philosopher is hungry and its neighbours are not eating.
 */
void test(int id)
{
    if (philosophers[id].state == HUNGRY && philosophers[LEFT(id)].state != EATING && philosophers[RIGHT(id)].state != EATING)
    {
        // both the forks are free --> start eating
        philosophers[id].state = EATING;
        // signal the philosopher's semaphore
        sem_post(&philosophers[id].sem);
    }
}