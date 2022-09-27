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

#include <sys/un.h>

#define SHOW_ITERATION // Show iteration count/progress

#define INPUT_FILE "brown.txt"


#define SOCK_PATH "./unix_sock.server"

void *map_file(char *filename, long *filesize);

// Ports
struct sockaddr_in get_server_address(size_t PORT);

void send_metadata(int fd, int numMessages, int remainderSize);
void send_messages(int fd, void *data, size_t messageSize, size_t numMessages, size_t remainderSize);
void send_kill(int fd);

int read_metadata(int socketfd, int *numMessages, int *remainder);
void read_messages(int fd, void *destbuffer, int messageSize);

void hc_write_loop(int outfd, int infd, void * data, size_t fileSize, size_t messageSize, size_t numMessages);
void hc_read_loop(int outfd, int infd, size_t messageSize, size_t numMessages);

void hc_latency_loop(int outfd, int infd, void * data, size_t messageSize, size_t numMessages, int isHost);
#endif
