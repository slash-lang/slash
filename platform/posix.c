#define _POSIX_SOURCE
#define _BSD_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <gc.h>
#include <string.h>
#include <pthread.h>
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

void*
pthread_get_stackaddr_np(pthread_t);

size_t
pthread_get_stacksize_np(pthread_t);

void*
sl_stack_limit()
{
    pthread_t self = pthread_self();
    void* addr = pthread_get_stackaddr_np(self);
    size_t size = pthread_get_stacksize_np(self);
    return (void*)((size_t)addr - size + 65536);
}