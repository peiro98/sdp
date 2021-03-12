/*
Exercise 06
UNIX threads and semaphores
---------------------------

Implements a concurrent C program, using semaphores, that:
- Generates one producer and one consumer thread, and it awaits for their
  termination.
- The producer threads produces, one at a time, 1000 integer numbers
  (from 0 to 999), and it puts each of them in a shared circular buffer of
  dimension BUFFER_SIZE (constant value, e.g., equal to 16), and finally
  puts -1.
- The consumer thread gets each number from the shared buffer, and it saves
  the number in file "out.txt". It terminates after reading -1. 

Verify that indeed file "out.txt" contains 1000 numbers, and that the
consumer thread receives these numbers in the correct order.

Suggestion
----------
Use the standard producer and consumer scheme with 1 single producer and 1
single consumer.
*/

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#define QUEUE_SIZE 16

typedef struct shared_queue_s {
    int data[QUEUE_SIZE];
    int head;
    int tail;

    sem_t empty;
    sem_t full;
} shared_queue_t;


void* producer (void *arg);

void* consumer (void *arg);


int main (int argc, char **argv) {
    pthread_t pc, pp;

    shared_queue_t queue;
    queue.head = queue.tail = 0;
    sem_init(&queue.empty, 0, QUEUE_SIZE);
    sem_init(&queue.full, 0, 0);

    pthread_create(&pc, NULL, &consumer, (void*) &queue);
    pthread_create(&pp, NULL, &producer, (void*) &queue);

    pthread_join(pc, NULL);
    pthread_join(pp, NULL);
}

void* producer (void *arg) {
    shared_queue_t* queue = (shared_queue_t*) arg;
    srand((int) getpid());

    for (int i = 0; i <= 1000; i++) {
        sem_wait(&queue->empty);

        // produce a new value
        queue->data[queue->tail] = (i < 1000) ? i : -1;
        queue->tail = (queue->tail + 1) % QUEUE_SIZE;

        sem_post(&queue->full);
    }
}

void* consumer (void *arg) {
    shared_queue_t* queue = (shared_queue_t*) arg;

    FILE *fp = fopen("numbers.txt", "w+");
    if (fp == NULL) {
        perror("Cannot open a new file numbers.txt");
        exit(1);
    }

    while (1) {
        sem_wait(&queue->full);

        // consume a value
        if (queue->data[queue->head] == -1) {
            break;
        }

        fprintf(fp, "%d\n", queue->data[queue->head]); 
        queue->head = (queue->head + 1) % QUEUE_SIZE;

        sem_post(&queue->empty);
    }

    fclose(fp);
}
