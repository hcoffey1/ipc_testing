/**********
 * Hayden Coffey
 *
 **********/
#include "util.h"

long FILE_SIZE;

void print_log_header()
{
    printf("===============================================\n");
    printf("TYPE: SOCKET\n");
    printf("MESSAGE SIZE : %lu B\n", MESSAGE_SIZE);
    printf("ITERATION COUNT: %lu\n", NUM_ITERATIONS);
    printf("-----\n");
    printf("Sending : %s\n", INPUT_FILE);
    printf("File size is : %lu B\n", FILE_SIZE);
    printf("Remainder data : %lu B\n", FILE_SIZE % MESSAGE_SIZE);
    printf("Preparing to send %lu messages at size %lu B...\n", FILE_SIZE / MESSAGE_SIZE, MESSAGE_SIZE);
    printf("===============================================\n");
}

// https://opensource.com/article/19/4/interprocess-communication-linux-networking

int main(int argc, char **argv)
{

    // Parse command line arguments----------
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s s/c\n", argv[0]);
        return 1;
    }

    // Determine if we are host/client and the port number
    int isHost = argv[1][0] == 's';
    size_t PORT = atoi(argv[2]);

    // Get socket FD
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
    {
        fprintf(stderr, "Failed to open socket.\n");
        return 1;
    }

    // Fill out server address socket struct
    struct sockaddr_in servAddr = get_server_address(PORT);

    // Host code---------------------
    if (isHost)
    {
        // Bind socket to host address
        if (bind(socketfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        //if (bind(socketfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        {
            perror("bind");
            return 1;
        }

        // Start listening for connections
        if (listen(socketfd, 1) < 0)
        {
            perror("listen");
            return 1;
        }

        fprintf(stderr, "Server: Listening on port %lu for connections.\n", PORT);

        void *data = map_file(INPUT_FILE, &FILE_SIZE);
        int numMessages = FILE_SIZE / MESSAGE_SIZE;
        int remainderSize = FILE_SIZE % MESSAGE_SIZE;

        print_log_header();
        // Host loop, accept new connections and run test
        while (1)
        {
            struct sockaddr_in clientAddr;
            socklen_t len = sizeof(clientAddr);

            int clientfd = accept(socketfd, (struct sockaddr *)&clientAddr, &len);
            if (clientfd < 0)
            {
                perror("accept");
                continue;
            }

            for (int i = 0; i < NUM_ITERATIONS; i++)
            {
                size_t offset = 0;

                // Tell reciever how many messages to expect
                write(clientfd, &numMessages, sizeof(numMessages));

                // Tell reciever if we need to send a message with the remaining data
                write(clientfd, &remainderSize, sizeof(remainderSize));

                // Send messages over socket
                for (int m = 0; m < numMessages; m++)
                {
                    write(clientfd, data + offset, MESSAGE_SIZE);
                    offset += MESSAGE_SIZE;
                }

                // Write remaining data if any
                if (remainderSize)
                {
                    write(clientfd, data + offset, remainderSize);
                }
            }

            int tmp = -1;
            write(clientfd, &tmp, sizeof(tmp));

            close(clientfd);
        }
    }
    // Client code----------------------
    else
    {
        // Connect to server address
        if (connect(socketfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        {
            perror("connect");
            return 1;
        }

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
                if (read(socketfd, &numMessages, sizeof(numMessages)) != 0)
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
            read(socketfd, &remainder, sizeof(remainder));

#ifdef SHOW_ITERATION
            printf("Iteration : %d\n", iteration);
#endif

            // Read in message
            int i = 0;
            while (i < numMessages)
            {
                if (read(socketfd, buf, MESSAGE_SIZE) != 0)
                {
                    i++;
                }
            }

            // Read remainder if needed
            if (remainder)
            {
                read(socketfd, buf, remainder);
            }

            iteration++;
        }
    }

    return 0;
}
