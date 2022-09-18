#ifndef _UTIL_H_
#define _UTIL_H_

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void *map_file(char *filename);

#endif