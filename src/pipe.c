/**********
 * Hayden Coffey
 *
 **********/
#include "util.h"

long FILE_SIZE;

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

    // Setup pipes------------------
    // Parent ---> Child messages
    int p1fd[2];

    // Child ---> Parent messages
    int p2fd[2];

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

    // Map sample data into memory--------
    void *data = map_file(INPUT_FILE, &FILE_SIZE);

    print_log_header();

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
        close(p1fd[0]);

        int numMessages = FILE_SIZE / MESSAGE_SIZE;
        int remainderSize = FILE_SIZE % MESSAGE_SIZE;
        for (int i = 0; i < NUM_ITERATIONS; i++)
        {
            size_t offset = 0;

            // Tell reciever how many messages to expect
            write(p1fd[1], &numMessages, sizeof(numMessages));

            // Tell reciever if we need to send a message with the remaining data
            write(p1fd[1], &remainderSize, sizeof(remainderSize));

            // Send messages over pipe
            for (int m = 0; m < numMessages; m++)
            {
                write(p1fd[1], data + offset, MESSAGE_SIZE);
                offset += MESSAGE_SIZE;
            }

            // Write remaining data if any
            if (remainderSize)
            {
                write(p1fd[1], data + offset, remainderSize);
            }
        }

        int tmp = -1;
        write(p1fd[1], &tmp, sizeof(tmp));
        close(p1fd[1]);

        wait(NULL);
    }

    // Child process---------
    else if (id == 0)
    {
        close(p1fd[1]);

        char buf[MESSAGE_SIZE];
        int numMessages;
        int remainder;
        int iteration = 0;
        while (1)
        {
            // Read in number of messages to expect
            int x;
            while (1)
            {
                if (read(p1fd[0], &numMessages, sizeof(numMessages)) != 0)
                {
                    break;
                }
            }

            // Termination signal
            if (numMessages < 0)
            {
                break;
            }

            // Read number of remaining bytes
            read(p1fd[0], &remainder, sizeof(remainder));

#ifdef SHOW_ITERATION
            printf("Iteration : %d\n", iteration);
#endif

            // Read in message
            int i = 0;
            while (i < numMessages)
            {
                if (read(p1fd[0], buf, MESSAGE_SIZE) != 0)
                {
                    i++;
                }
            }

            // Read remainder if needed
            if (remainder)
            {
                read(p1fd[0], buf, remainder);
            }

            iteration++;
        }

        close(p1fd[0]);
    }

    return 0;
}
