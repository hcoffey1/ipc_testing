#include "util.h"

// Map the specified file into memory and return the starting address
void *map_file(char *filename, long * filsesize)
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

    *filsesize=filestat.st_size;
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

    servAddr.sin_family = AF_LOCAL;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(PORT);

	return servAddr;
}