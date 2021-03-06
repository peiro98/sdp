/*
 * The program must spawn two child processes. Each process acyclically performs
 * some work and sends a SIGUSR1 (or SIGUSR2) signal to the father.
 * 
 * When three identical signals are received, the fathers kills the child processes
 * and returns.
 */


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

void child(int i);

int completed = 0;

void handler(int sig)
{
    static int arr[3] = {-1, -1, -1};
    static int pos = 0;

    arr[pos] = sig;
    fprintf(stdout, "Received signal from %d\n", sig);
    
    if (arr[pos] == arr[(pos + 1) % 3] && arr[pos] == arr[(pos + 2) % 3]) {
        // last three signals are equal
        completed = 1;
    }

    pos = (pos + 1) % 3;
}

int main(int argc, char **argv)
{
    pid_t pids[2];
    srand(getpid());

    if (signal(SIGUSR1, &handler) == SIG_ERR || signal(SIGUSR2, &handler) == SIG_ERR) {
        fprintf(stderr, "Error while setting the signal handler\n");
        return 2;
    }

    for (int i = 0; i < 2; i++) {
        if (!(pids[i] = fork())) {
            child(i);
            return 0;
        }
    }

    while (!completed) pause();
    fprintf(stdout, "Received a complete sequence!\n");
    if (kill(pids[0], SIGKILL) || kill(pids[0], SIGKILL)) {
        fprintf(stderr, "Error while killing the processes\n");
        return 1;
    }

    return 0;
}

void child(int id)
{
    int max = id * 10;
    while (1) {
        sleep((int)(rand() % max) + 1);
        kill(getppid(), id == 1 ? SIGUSR1 : SIGUSR2);
    }
}
