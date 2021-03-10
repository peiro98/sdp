/*
Exercise 02 (Optional)
UNIX signals
----------------------

Write a C program that is able to handle signals as described by the following
specifications.
The program accepts signals SIGUSR1 and SIGUSR2, and:
- It displays a "success" message every time it receives a signal SIGUSR1 followed
  by a signal SIGUSR2, or viceversa.
- It displays an error message every time it receives two signals SIGUSR1 or two
  signals SIGUSR2 consecutively.
- It terminates if it receives three successive SIGUSR1 or SIGUSR2 signals.

Suggestion
-----------
Once it is compiled, run the program in background (...&) and use the shell
command "kill" to send signal SIGUSR1 and SIGUSR2 to the process.
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
