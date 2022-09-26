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

void send_metadata(int fd, int numMessages, int remainderSize)
{
    // Tell receiver how many messages to expect
    write(fd, &numMessages, sizeof(numMessages));

    // Tell receiver if we need to send a message with the remaining data
    write(fd, &remainderSize, sizeof(remainderSize));
}

void send_messages(int fd, void *data, size_t messageSize, size_t numMessages, size_t remainderSize)
{
    size_t offset = 0;

    // Send messages over socket
    for (int m = 0; m < numMessages; m++)
    {
        write(fd, data + offset, messageSize);
        offset += messageSize;
    }

    // Write remaining data if any
    if (remainderSize)
    {
        write(fd, data + offset, remainderSize);
    }
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

void read_messages(int fd, void *destbuffer, int messageSize, int numMessages, int remainder)
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

void hc_write_loop(int outfd, void * data, size_t fileSize, size_t messageSize, size_t numIterations)
{
    int numMessages = fileSize / messageSize;
    int remainderSize = fileSize % messageSize;

    // Work loop
    for (int i = 0; i < numIterations; i++)
    {
        // Let the client know how many messages to expect
        send_metadata(outfd, numMessages, remainderSize);

        // Send the messages
        send_messages(outfd, data, messageSize, numMessages, remainderSize);
    }

    // Close client
    send_kill(outfd);
}

void hc_read_loop(int infd, size_t messageSize)
{
    int numMessages;
    int remainder;
    int iteration = 0;
    char *buf = malloc(messageSize);
    while (1)
    {
        // Read in how many messages to expect
        if (read_metadata(infd, &numMessages, &remainder) == -1)
        {
            break;
        }

#ifdef SHOW_ITERATION
        printf("Iteration : %d\n", iteration);
#endif

        // Read messages from host
        read_messages(infd, buf, messageSize, numMessages, remainder);

        iteration++;
    }
    free(buf);
}
