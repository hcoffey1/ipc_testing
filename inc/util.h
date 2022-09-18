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

#include <arpa/inet.h>
#include <netdb.h>

//#define PORT 8073

void *map_file(char *filename);

//Ports
struct sockaddr_in get_server_address(size_t PORT);

#endif