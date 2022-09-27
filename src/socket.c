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

    size_t NUM_MESSAGES = 1ul << 10;

    // Host code---------------------
    if (isHost)
    {

        NUM_ITERATIONS = 1ul << atoi(argv[4]);

        void *data = map_file(INPUT_FILE, &FILE_SIZE);

        // Print log information at top of run
        print_log_header();

        // Open server and get fd pointing to client
        int clientfd = host_server(PORT);

        // Example work loop
        hc_write_loop(clientfd, clientfd, data, FILE_SIZE, MESSAGE_SIZE, NUM_ITERATIONS, NUM_MESSAGES);
        hc_latency_loop(clientfd, clientfd, data, MESSAGE_SIZE, NUM_MESSAGES, 1);
    }
    // Client code----------------------
    else
    {
        // Connect to server
        int hostfd = client_connect(PORT);

        // Example work loop
        hc_read_loop(hostfd, hostfd, MESSAGE_SIZE, NUM_MESSAGES);
        hc_latency_loop(hostfd, hostfd, NULL, MESSAGE_SIZE, NUM_MESSAGES, 0);
    }

    return 0;
}
