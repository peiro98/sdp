/*
Exercise 05
UNIX threads
------------

Implement a C program, thread_generation, that receives a command line parameter
n.
The parent thread creates two threads and waits for their termination.
Each further thread creates other two threads, and it awaits for their termination. 
Tread creation stops after 2^n threads have been created, i.e., the ones that stand
on the leaves of a tree with 2^n leaves.

For example, if n=3
- the main thread creates two threads
- each one of these 2 threads creates other two threads,
- each one of these 4 threads creates other two threads
at this point 8 leaf treads are running and the program must stop.

Each leaf thread must print its generation tree, i.e., the sequence of thread
identifiers from the main thread (the tree root) to the leaf thread (tree leaf).

The following is an example of the program execution: 

quer@quer-VirtualBox:~/current/sdp$ ./l01e05 3
140051327870720 140051311085312 140051224717056 
140051327870720 140051311085312 140051224717056 
140051327870720 140051311085312 140051233109760 
140051327870720 140051319478016 140051207931648 
140051327870720 140051311085312 140051233109760 
140051327870720 140051319478016 140051207931648 
140051327870720 140051319478016 140051216324352 
140051327870720 140051319478016 140051216324352 

Suggestion
----------
Print (and store) thread identifiers (tids) as "long integer"
values.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct data_s {
    pthread_t *array;
    int index;
    int depth;
} data_t;


void create_threads (pthread_t *threads, int index, int depth);

sem_t mutex;

int main (int argc, char **argv) {
    // array containing a pthread_t for each created thread
    pthread_t *threads;

    if (argc != 2) {
        printf("USAGE: ./a.out n\n");
        return 1;
    }

    int n = atoi(argv[1]);

    threads = (pthread_t*) malloc((2<<(n+1) - 1) * sizeof(pthread_t));
    if (threads == NULL) {
        perror("Allocation error\n");
        return 1;
    }

    sem_init(&mutex, 0, 1);
    threads[0] = pthread_self();
    create_threads(threads, 0, n);

    free(threads);
}

void print_tree (pthread_t *threads, int index) {
    if (index > 0) {
        print_tree(threads, (index - 1) / 2);
    }
    printf("%ld ", *(threads + index));
}

void* thread_foo (void *arg) {
    data_t *data = (data_t*) arg;

    create_threads(data->array, data->index, data->depth);
}

void create_threads (pthread_t *threads, int index, int depth) {
    if (depth == 0) {
        sem_wait(&mutex);
        print_tree(threads, (index - 1) / 2);
        printf("\n");
        sem_post(&mutex);
        return;
    }

    data_t data[2];

    for (int i = 0; i < 2; i++) {
        data[i].array = threads;
        data[i].index = 2 * index + i + 1;
        data[i].depth = depth - 1;
        pthread_create(threads + 2 * index + i + 1, NULL, &thread_foo, &data[i]);
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(*(threads + 2 * index + i + 1), NULL);
    }
}

