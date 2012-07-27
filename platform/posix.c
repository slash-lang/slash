#define _POSIX_SOURCE
#define _BSD_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <gc.h>
#include <string.h>
#include "platform.h"

char*
sl_realpath(char* path)
{
    #ifdef PATH_MAX
        char* cpath = GC_MALLOC(PATH_MAX + 1);
        realpath(path, cpath);
        return cpath;
    #else
        char* cpath = realpath(path, NULL);
        char* gcbuff = GC_MALLOC(strlen(cpath) + 1);
        strcpy(gcbuff, cpath);
        return gcbuff;
    #endif
}

int
sl_file_exists(char* path)
{
    struct stat s;
    return !stat(path, &s);
}

int sl_seed()
{
    struct timeval a;
    gettimeofday(&a, NULL);
    return (int)(a.tv_usec ^ a.tv_sec);
}