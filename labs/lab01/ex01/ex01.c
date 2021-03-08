/*
Exercise 01 (Optional)
The UNIX file system
----------------------

An ASCII file has lines with the following format:
- An identifier, i.e., an integer value varying from 1 to the number
  of rows in the file (e.g., 1, 2, etc.)
- A register number, i.e., a long integer of 6 digits (e.g., 164678)
- A surname, i.e., a string of maximum 30 characters (e.g., Rossi)
- A name, i.e., a string of maximum 30 characters (e.g., Mario)
- An examination mark, i.e., an integer value.

The following is a correct example of such a file:

1 100000 Romano Antonio 25
2 150000 Fabrizi Aldo 22
3 200000 Verdi Giacomo 15
4 250000 Rossi Luigi 30

Write a C program in UNIX which is run with 3 file names on the
command line, i.e.,

l01file  file_input_1  file_output_1  file_output_2

and which it is able to:
- Read the *text* file file_input_1 (with the previous format) and
  copy it into file file_output_1 in *binary* format and with
  *fixed-length* records
  (integer + long integer + 31 characters + 31 characters + integer).
- Read back the *binary* file file_output_1 and store it as a *text*
  into file file_output_2.
- Verify that files file_input_1 and file_output_2 are indeed equivalent
  using the shell command "wc" (word count) and "diff".

Suggestions
-----------
- Manipulate text files using C library function
- Manipulate binary files using the I/O UNIX library
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

#include <errno.h>

#define STR_LENGTH (30 + 1)

typedef struct
{
    int id;
    int register_value;
    char surname[STR_LENGTH];
    char name[STR_LENGTH];
    int score;
} exam_t;

int to_binary(char *input_file, char *output_file);
int from_binary(char *input_file, char *output_file);

int main(int argc, char **argv)
{
    int res;

    if (argc != 4)
    {
        fprintf(stderr, "USAGE: input.txt output_1.txt output2.txt\n");
        return 1;
    }

    res = to_binary(argv[1], argv[2]);
    if (!res)
        res |= from_binary(argv[2], argv[3]);

    return res ? 1 : 0;
}

int to_binary(char *input_file, char *output_file)
{
    int n;
    exam_t buf = {0};

    FILE *in = fopen(input_file, "r");

    if (in == NULL)
    {
        fprintf(stderr, "Error while opening the input file %s with errno %d\n", input_file, errno);
        return 1;
    }

    int out = open(output_file, O_RDWR | O_CREAT | O_TRUNC, 0770);

    if (out < 0)
    {
        fprintf(stderr, "Error while opening the output file %s with errno %d\n", output_file, errno);
        fclose(in);
        return 1;
    }

    while (!feof(in))
    {
        n = fscanf(in, "%d %6d %s %s %d", &buf.id, &buf.register_value, (char *)&buf.surname, (char *)&buf.name, &buf.score);

        // skip lines that do not contain 5 values
        if (n != 5)
        {
            if (n == 0)
                fprintf(stderr, "Error while reading the input file %s with errno %d\n", input_file, errno);
            break;
        }

        n = write(out, &buf, sizeof(exam_t));

        // check if the writing operation was successful
        if (n != sizeof(exam_t))
        {
            fprintf(stderr, "Error while writing the output file %s with errno %d\n", output_file, errno);

            // close the file
            fclose(in);
            close(out);
            return 1;
        }
    }

    // close the file
    fclose(in);
    close(out);

    return 0;
}

int from_binary(char *input_file, char *output_file)
{
    int n;
    exam_t buf;

    int in = open(input_file, O_RDONLY);

    // open returns -1 in case of error
    if (in < 0)
    {
        fprintf(stderr, "Error while opening the input file %s with errno %d\n", input_file, errno);
        return 1;
    }

    FILE *out = fopen(output_file, "w+");

    if (out == NULL)
    {
        fprintf(stderr, "Error while opening the output file %s with errno %d\n", output_file, errno);
        close(in);
        return 1;
    }

    while ((n = read(in, &buf, sizeof(exam_t)) != sizeof(exam_t)))
    {
        n = fprintf(out, "%d %6d %s %s %d\n", buf.id, buf.register_value, buf.surname, buf.name, buf.score);
        // fprintf returns -1 in case of error
        if (n < 0)
        {
            fprintf(stderr, "Error while writing to the output file %s with errno %d\n", output_file, errno);
            break;
        }
    }

    // check if the loop exited due to an error
    if (n < 0)
    {
        fprintf(stderr, "Error while reading from the input file %s %d\n", input_file, errno);
    }

    // close the files
    close(in);
    fclose(out);

    return 0;
}