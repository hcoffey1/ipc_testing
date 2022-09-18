/**********
 * Hayden Coffey
 *
 **********/
#include "util.h"

// https://opensource.com/article/19/4/interprocess-communication-linux-networking

int main(int argc, char **argv)
{

    // Parse command line arguments----------
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s s/c port\n", argv[0]);
        return 1;
    }

    // Determine if we are host/client and the port number
    int isHost = argv[1][0] == 's';
    size_t PORT = atoi(argv[2]);

    // Get socket FD
    int socketfd = socket(AF_LOCAL, SOCK_STREAM, 0);
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

        // Host loop, accept new connections and recieve messages from clients
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

            for (int i = 0; i < 10; i++)
            {
                char buffer[257];
                memset(buffer, '\0', 257);
                int count = read(clientfd, buffer, 256);
                if (count > 0)
                {
                    puts(buffer);
                    write(clientfd, buffer, 256);
                }
            }
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

        // Send data
        puts("Connected to server, sending message.");

        write(socketfd, "Hello world!", strlen("Hello world!"));

        puts("Client: Done. Exiting...");
    }

    return 0;
}
