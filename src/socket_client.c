#include "util.h"

int main()
{

	int sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);

	if(sockfd < 0)
	{
		perror("Socket");
		return 1;
	}

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));

    servAddr.sin_family = AF_LOCAL;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(PORT);

	if(connect(sockfd, (struct sockaddr*) &servAddr,sizeof(servAddr)) < 0)
	{
		perror("connect");
		return 1;
	}

	puts("Connected to server, sending message.");

	write(sockfd, "Hello world!", strlen("Hello world!"));

	puts("Client: Done. Exiting...");

	//close(sockfd);
	//if(hptr->h_addrtype != AF_LOCAL)
	//{
	//	perror("Bad address family.");
	//	return 1;
	//}

}