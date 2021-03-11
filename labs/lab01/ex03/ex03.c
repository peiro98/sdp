/*
Exercise 03 (Optional)
UNIX pipes
----------------------

Two processes P1 and P2 want to exchange some data (in both directions) using a file,
such that what it is written by P1 into the file is read by P2 and vice-versa.
To be able to read and write the same file, P1 and P2 must synchronize their R/W
operations.
They decided to do that using a pipe with the follwoing specifications.

Initially, an ASCII file stores a set of strings, one string on each file row.

The C program runs two processes P1 and P2, such that:
- Process P1 starts (while process P2 awaits) and it:
  - Reads the file.
  - Displays the file content on standard output.
  - Reads a new set of strings from standard input and it stores them
    into the same file with the same format.
    The input phase ends when the string "end" or the strign "END" are introduced.
  - In the first case (string "end" is introduced) P1 wakes-up process P2 and goes into a
    wait state. In the second case (string "END" is introduced) P1 must terminate
    itself and P2 (directly or indirectly).
- Process P2 starts (while process P1 awaits) and it proceeds as process P1.
  At the end of its job, it wakes-up process P1, to start a new cycle, or it makes
  P1 to terminate.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define BUFSIZE 256

void p1(int pipe_no1[2], int pipe_no2[2], FILE *fp, pid_t child);
void p2(int pipe_no1[2], int pipe_no2[2], FILE *fp);

void display_file (FILE *fp);
int write_file (FILE *fp);

void bye () {
    fprintf(stdout, "\n[%d]: bye!\n", getpid());
}

int main (int argc, char **argv) {
    int pipe_no1[2], pipe_no2[2];

    if (atexit(&bye)) {
        perror("Cannot bind the signal handler\n");
        return 1;
    }

    FILE *fp = tmpfile();
    if (fp == NULL) {
        perror("Cannot open a tmp file\n");
        return 1;
    }

    if (pipe(pipe_no1) || pipe(pipe_no2)) {
        perror("Cannot open one or more pipes\n");
        return 1;
    }

    pid_t child;
    if (child = fork()) {
        fprintf(stdout, "[%d]: child process has pid = %d\n", getpid(), child);
        p1(pipe_no1, pipe_no2, fp, child);
        wait(NULL);
    } else {
        p2(pipe_no1, pipe_no2, fp);
    }

}

void p1 (int pipe_no1[2], int pipe_no2[2], FILE *fp, pid_t child) {
    // close the unused extremes of the pipes
    close(pipe_no1[0]);
    close(pipe_no2[1]);

    int loop = 1;
    while (loop) {
        loop = write_file(fp);
        write(pipe_no1[1], &loop, sizeof(int));

        if (loop) {
            // wait for the other process
            read(pipe_no2[0], &loop, sizeof(int));
            if (loop) display_file(fp);
        }
    }
}

void p2 (int pipe_no1[2], int pipe_no2[2], FILE *fp) {
    // close the writing extreme of the pipe
    close(pipe_no1[1]);
    close(pipe_no2[0]);

    int loop = 1;
    while (loop) {
        // wait for the other process
        read(pipe_no1[0], &loop, sizeof(int));
        if (loop) {
            // print the content of the file
            display_file(fp);
            // write something on the file
            loop = write_file(fp);
            write(pipe_no2[1], &loop, sizeof(int));
        }
    }
}

int write_file (FILE *fp) {
    char buffer[BUFSIZE] = { 0 };

    rewind(fp);

    fprintf(stdout, "\n[%d]: writing to the file (terminate with 'end' or 'END'):\n", getpid());

    while (strcmp(buffer, "end\n") && strcmp(buffer, "END\n")) {
        fprintf(stdout, ">>> ");
        if (fgets(buffer, BUFSIZE, stdin)) {
            fputs(buffer, fp);
        }
    }

    // flush the buffer
    fflush(fp);
    
    return (strcmp(buffer, "end\n") == 0) ? 1 : 0;
}

void display_file (FILE *fp) {
    char buf[BUFSIZE] = { 0 };

    rewind(fp);

    fprintf(stdout, "\n[%d]: printing the content of the file: \n", getpid());

    int loop = 1;
    while (!feof(fp) && loop) {
        if (fgets((char*) &buf, BUFSIZE, fp)) {
            if (!strcmp(buf, "end\n") || !strcmp(buf, "END\n")) {
                loop = 0;
            } else {
                fprintf(stdout, ">>> %s", buf);
            }
        }
    }
}
