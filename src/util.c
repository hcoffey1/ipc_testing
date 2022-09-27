#include "util.h"

// Map the specified file into memory and return the starting address
void *map_file(char *filename, long *filsesize)
{
    FILE *fp = fopen(filename, "r");
    if (fp < 0)
    {
        fprintf(stderr, "Failed to open file : %s\n", filename);
        exit(-1);
    }

    struct stat filestat;

    if (fstat(fileno(fp), &filestat) != 0)
    {
        fprintf(stderr, "Failed to stat file\n");
        exit(-1);
    }

    *filsesize = filestat.st_size;
    (*filsesize)++;

    void *mapaddr = mmap(NULL, filestat.st_size, PROT_READ, MAP_PRIVATE, fileno(fp), 0);

    if (mapaddr == MAP_FAILED)
    {
        fprintf(stderr, "Failed to map file\n");
        exit(-1);
    }

    fclose(fp);

    return mapaddr;
}

struct sockaddr_in get_server_address(size_t PORT)
{
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(PORT);

    return servAddr;
}

void send_ack(int fd)
{
    int tmp = 1;
    write(fd, &tmp, sizeof(tmp));
}

void read_ack(int fd)
{
    int tmp;
    read(fd, &tmp, sizeof(tmp));
}

void send_metadata(int fd, int numMessages, int remainderSize)
{
    // Tell receiver how many messages to expect
    write(fd, &numMessages, sizeof(numMessages));

    // Tell receiver if we need to send a message with the remaining data
    write(fd, &remainderSize, sizeof(remainderSize));
}

void send_messages(int fd, void *data, size_t messageSize, size_t numMessages, size_t remainderSize)
{
    //size_t offset = 0;

    // Send messages over socket
    printf("Starting\n");
    for (int m = 0; m < numMessages; m++)
    {
        write(fd, data, messageSize);
    }

    printf("Done\n");
}

void send_kill(int fd)
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

void read_messages(int fd, void *destbuffer, int messageSize)
{
    // Read in message
    int i = 0;
    while (1)
    {
        int y = read(fd, destbuffer, messageSize);
        if (y == 0)
        {
            break;
        }
    }
}

//Can only read 64kb at once from fd
#define BUFFER_LIMIT (1<<16)

void hc_latency_loop(int outfd, int infd, void * data, size_t messageSize, size_t numMessages, int isHost)
{
    char * inbuf = malloc(messageSize);
    if(isHost)
    {
        printf("Running latency test\n");
        for(int i = 0; i < numMessages; i++)
        {
            write(outfd, data, messageSize);

            read(infd, inbuf, messageSize);
            for(int j = 0; j < messageSize/BUFFER_LIMIT; j++)
                read(infd, inbuf, messageSize);
        }
        printf("Done\n");
    }
    else
    {
        for(int i = 0; i < numMessages; i++)
        {
            //Perform multiple reads if necessary for large writes
            read(infd, inbuf, messageSize);
            for(int j = 0; j < messageSize/BUFFER_LIMIT; j++)
                read(infd, inbuf, messageSize);

            write(outfd, inbuf, messageSize);
        }
    }
}

void hc_write_loop(int outfd, int infd, void * data, size_t fileSize, size_t messageSize, size_t numMessages)
{
    printf("Sending num messages: %lu\n", numMessages);
    printf("Sending message size: %lu\n", messageSize);

    // Send the messages
    send_messages(outfd, data, messageSize, numMessages, 0);
    close(outfd);

    read_ack(infd);

    printf("Recieved ack\n");
}


void hc_read_loop(int outfd, int infd, size_t messageSize, size_t numMessages)
{
    int remainder;
    int iteration = 0;
    char *buf = malloc(messageSize);

    // Read messages from host
    read_messages(infd, buf, messageSize);

    printf("Sending ack\n");
    send_ack(outfd);

    free(buf);
}
