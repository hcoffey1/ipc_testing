/**********
 * Hayden Coffey
 * 
 **********/
#include "util.h"

//https://opensource.com/article/19/4/interprocess-communication-linux-networking

int main(int argc, char ** argv)
{

    int socketfd = socket(AF_LOCAL, SOCK_STREAM, 0);

    if(socketfd < 0)
    {
        fprintf(stderr, "Failed to open socket.\n");
        return 1;
    }

    int opt=1;

 // Forcefully attaching socket to the port 8080
    //if (setsockopt(socketfd, SOL_SOCKET,
    //               SO_REUSEADDR | SO_REUSEADDR, &opt,
    //               sizeof(opt))) {
    //    perror("setsockopt");
    //    return 1;
    //}

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));

    servAddr.sin_family = AF_LOCAL;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(PORT);

    if(bind(socketfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
    {
        //fprintf(stderr, "Bind failure.\n");
        perror("bind");
        return 1;
    }

    if(listen(socketfd, 1) < 0)
    {
        perror("listen");
        return 1;
    }

    fprintf(stderr, "Server: Listening on port %d for connections.\n", PORT);

    while(1)
    {
        struct sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);

        int clientfd = accept(socketfd, (struct sockaddr*) &clientAddr, &len);
        if(clientfd < 0)
        {
            perror("accept");
            continue;
        }

        for(int i = 0; i < 10; i++)
        {
            char buffer[257];
            memset(buffer, '\0', 257);
            int count = read(clientfd, buffer, 256);
            if(count > 0)
            {
                puts(buffer);
                write(clientfd, buffer, 256);
            }
        }
        close(clientfd);
    }
  
    return 0;
}

