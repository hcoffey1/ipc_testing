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

int open_socket()
{
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
    {
        fprintf(stderr, "Failed to open socket.\n");
        return 1;
    }

    return socketfd;
}

int host_server(size_t PORT)
{
    // Get socket FD
    int socketfd = open_socket();

    // Fill out server address socket struct
    struct sockaddr_in servAddr = get_server_address(PORT);

    // Bind socket to host address
    if (bind(socketfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    // if (bind(socketfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    // Start listening for connections
    if (listen(socketfd, 1) < 0)
    {
        perror("listen");
        exit(1);
    }

    fprintf(stderr, "Server: Listening on port %lu for connections.\n", PORT);

    // Host loop, accept new connections and run test
    struct sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);

    int clientfd = accept(socketfd, (struct sockaddr *)&clientAddr, &len);
    if (clientfd < 0)
    {
        perror("accept");
        exit(1);
    }

    return clientfd;
}

int client_connect(size_t PORT)
{
    // Get socket FD
    int socketfd = open_socket();

    // Fill out server address socket struct
    struct sockaddr_in servAddr = get_server_address(PORT);

    // Connect to server address
    if (connect(socketfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        perror("connect");
        exit(1);
    }

    return socketfd;
}

void send_messages(void *data, int fd, size_t messageSize, size_t numMessages, size_t remainderSize)
{
    size_t offset = 0;

    // Send messages over socket
    for (int m = 0; m < numMessages; m++)
    {
        write(fd, data + offset, MESSAGE_SIZE);
        offset += MESSAGE_SIZE;
    }

    // Write remaining data if any
    if (remainderSize)
    {
        write(fd, data + offset, remainderSize);
    }
}

void send_metadata(int numMessages, int remainderSize, int fd)
{
    // Tell reciever how many messages to expect
    write(fd, &numMessages, sizeof(numMessages));

    // Tell reciever if we need to send a message with the remaining data
    write(fd, &remainderSize, sizeof(remainderSize));
}

void send_kill(fd)
{
    int tmp = -1;
    write(fd, &tmp, sizeof(tmp));
    close(fd);
}

int read_metadata(int socketfd, int *numMessages, int *remainder)
{
    // Read in number of messages to expect
    while (1)
    {
        if (read(socketfd, numMessages, sizeof(*numMessages)) != 0)
        {
            break;
        }
    }

    // Termination signal
    if (*numMessages < 0)
    {
        return -1;
    }

    // Read number of remaining bytes
    read(socketfd, remainder, sizeof(*remainder));

    return 0;
}

void read_messages(void *destbuffer, int fd, int messageSize, int numMessages, int remainder)
{
    // Read in message
    int i = 0;
    while (i < numMessages)
    {
        if (read(fd, destbuffer, messageSize) != 0)
        {
            i++;
        }
    }

    // Read remainder if needed
    if (remainder)
    {
        read(fd, destbuffer, remainder);
    }
}

// https://opensource.com/article/19/4/interprocess-communication-linux-networking

int main(int argc, char **argv)
{

    // Parse command line arguments----------
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s s/c message_size iterations\n", argv[0]);
        return 1;
    }

    // Determine if we are host/client and the port number
    int isHost = argv[1][0] == 's';

    if (isHost && argc != 5)
    {
        fprintf(stderr, "Usage: %s s/c message_size iterations(if server)\n", argv[0]);
        return 1;
    }
    else if (argc < 4)
    {
        fprintf(stderr, "Usage: %s s/c message_size iterations(if server)\n", argv[0]);
    }

    size_t PORT = atoi(argv[2]);
    MESSAGE_SIZE = 1ul << atoi(argv[3]);

    // Host code---------------------
    if (isHost)
    {

        NUM_ITERATIONS = 1ul << atoi(argv[4]);

        void *data = map_file(INPUT_FILE, &FILE_SIZE);
        int numMessages = FILE_SIZE / MESSAGE_SIZE;
        int remainderSize = FILE_SIZE % MESSAGE_SIZE;

        // Print log information at top of run
        print_log_header();

        // Open server and get fd pointing to client
        int clientfd = host_server(PORT);

        // Work loop
        for (int i = 0; i < NUM_ITERATIONS; i++)
        {
            // Let the client know how many messages to expect
            send_metadata(numMessages, remainderSize, clientfd);

            // Send the messages
            send_messages(data, clientfd, MESSAGE_SIZE, numMessages, remainderSize);
        }

        // Close client
        send_kill(clientfd);
    }
    // Client code----------------------
    else
    {
        // Connect to server
        int hostfd = client_connect(PORT);

        // Example work loop
        char *buf = malloc(MESSAGE_SIZE);
        int numMessages;
        int remainder;
        int iteration = 0;
        while (1)
        {
            // Read in how many messages to expect
            if (read_metadata(hostfd, &numMessages, &remainder) == -1)
            {
                break;
            }

#ifdef SHOW_ITERATION
            printf("Iteration : %d\n", iteration);
#endif

            // Read messages from host
            read_messages(buf, hostfd, MESSAGE_SIZE, numMessages, remainder);

            iteration++;
        }
    }

    return 0;
}
