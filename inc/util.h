#ifndef _UTIL_H_
#define _UTIL_H_

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <netdb.h>

#define SHOW_ITERATION // Show iteration count/progress

#define INPUT_FILE "brown.txt"

#define MESSAGE_SIZE (1ul << 8)

#define NUM_ITERATIONS 1ul << 4

#define SHARED_BUFFER_SIZE (MESSAGE_SIZE) // Shared Buffer Only

void *map_file(char *filename, long *filesize);

// Ports
struct sockaddr_in get_server_address(size_t PORT);

#endif