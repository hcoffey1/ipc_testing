/**********
 * Hayden Coffey
 *
 **********/
#include "util.h"

// https://opensource.com/article/19/4/interprocess-communication-linux-networking

int main(int argc, char **argv)
{

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s s/c port\n", argv[0]);
        return 1;
    }

    int isHost = argv[1][0] == 's';

    size_t PORT = atoi(argv[2]);

    int socketfd = socket(AF_LOCAL, SOCK_STREAM, 0);

    if (socketfd < 0)
    {
        fprintf(stderr, "Failed to open socket.\n");
        return 1;
    }

    struct sockaddr_in servAddr = get_server_address(PORT);

    if (isHost)
    {
        if (bind(socketfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        {
            // fprintf(stderr, "Bind failure.\n");
            perror("bind");
            return 1;
        }

        if (listen(socketfd, 1) < 0)
        {
            perror("listen");
            return 1;
        }

        fprintf(stderr, "Server: Listening on port %lu for connections.\n", PORT);

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
    else
    {
        if (connect(socketfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        {
            perror("connect");
            return 1;
        }

        puts("Connected to server, sending message.");

        write(socketfd, "Hello world!", strlen("Hello world!"));

        puts("Client: Done. Exiting...");
    }

    return 0;
}
