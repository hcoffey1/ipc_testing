/**********
 * Hayden Coffey
 *
 **********/
#include "util.h"

long FILE_SIZE;
size_t MESSAGE_SIZE;
size_t NUM_ITERATIONS;

void print_log_header()
{
    printf("===============================================\n");
    printf("TYPE: PIPE\n");
    printf("MESSAGE SIZE : %lu B\n", MESSAGE_SIZE);
    printf("-----\n");
    printf("Sending : %s\n", INPUT_FILE);
    printf("File size is : %lu B\n", FILE_SIZE);
    printf("Remainder data : %lu B\n", FILE_SIZE % MESSAGE_SIZE);
    printf("Preparing to send %lu messages at size %lu B...\n", FILE_SIZE / MESSAGE_SIZE, MESSAGE_SIZE);
    printf("===============================================\n");
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s message_size iterations\n", argv[0]);
        return 1;
    }

    MESSAGE_SIZE = 1ul << atoi(argv[1]);
    NUM_ITERATIONS = 1ul << atoi(argv[2]);

    // Setup pipes------------------
    // Parent ---> Child messages
    int p1fd[2];

    // Child ---> Parent messages
    // int p2fd[2];

    if (pipe(p1fd) == -1)
    {
        fprintf(stderr, "Failed to create pipe.\n");
        return 1;
    }

    // if (pipe(p2fd) == -1)
    //{
    //     fprintf(stderr, "Failed to create pipe.\n");
    //     return 1;
    // }

    // Fork child process----------------
    pid_t id;
    id = fork();

    if (id < 0)
    {
        fprintf(stderr, "Fork failed\n");
        return 1;
    }

    // Parent process--------
    else if (id != 0)
    {
        // Close reading end of pipe
        close(p1fd[0]);

        // Map sample data into memory--------
        void *data = map_file(INPUT_FILE, &FILE_SIZE);

        print_log_header();

        // Example work loop
        hc_write_loop(p1fd[1], data, FILE_SIZE, MESSAGE_SIZE, NUM_ITERATIONS);

        wait(NULL);
    }

    // Child process---------
    else if (id == 0)
    {
        // Close writing end of pipe
        close(p1fd[1]);

        // Example work loop
        hc_read_loop(p1fd[0], MESSAGE_SIZE);

        close(p1fd[0]);
    }

    return 0;
}
