/**********
 * Hayden Coffey
 *
 **********/
#include "util.h"

long FILE_SIZE;
size_t MESSAGE_SIZE;

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

// Setup pipes------------------
// Parent ---> Child messages
int p1fd[2];

// Child ---> Parent messages
int p2fd[2];

void init_parent_fd()
{
    // Close reading end of pipe
    close(p1fd[0]);

    //Close writing end of second pipe
    close(p2fd[1]);
}

void init_child_fd()
{
    // Close writing end of pipe
    close(p1fd[1]);

    //Close reading end of second pipe
    close(p2fd[0]);
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s message_size iterations\n", argv[0]);
        return 1;
    }

    MESSAGE_SIZE = 1ul << atoi(argv[1]);
    size_t NUM_MESSAGES = 1ul << 10;


    if (pipe(p1fd) == -1)
    {
        fprintf(stderr, "Failed to create pipe.\n");
        return 1;
    }

    if (pipe(p2fd) == -1)
    {
        fprintf(stderr, "Failed to create pipe.\n");
        return 1;
    }
    // Map sample data into memory--------
    void *data = map_file(INPUT_FILE, &FILE_SIZE);

    int numMessages = 1ul << 10;

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
        init_parent_fd();

        int writetochild_fd = p1fd[1];
        int readfromchild_fd = p2fd[0];

        // Example work loop
        hc_write_loop(writetochild_fd, readfromchild_fd, data, FILE_SIZE, MESSAGE_SIZE, numMessages);
        hc_latency_loop(writetochild_fd, readfromchild_fd, data, MESSAGE_SIZE, numMessages, 1);

        wait(NULL);
    }

    // Child process---------
    else if (id == 0)
    {
        init_child_fd();

        int writetohost_fd = p2fd[1];
        int readfromhost_fd = p1fd[0];

        // Example work loop
        hc_read_loop(writetohost_fd, readfromhost_fd, MESSAGE_SIZE, numMessages);
        hc_latency_loop(writetohost_fd, readfromhost_fd, NULL, MESSAGE_SIZE, numMessages, 0);

        close(readfromhost_fd);
    }


    return 0;
}
