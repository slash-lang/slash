#define _POSIX_SOURCE
#define _BSD_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "platform.h"

char*
sl_realpath(sl_vm_t* vm, char* path)
{
    char *cpath, *gcbuff;
    if(path[0] != '/') {
        gcbuff = sl_alloc_buffer(vm->arena, strlen(vm->cwd) + strlen(path) + 10);
        strcpy(gcbuff, vm->cwd);
        strcat(gcbuff, "/");
        strcat(gcbuff, path);
        path = gcbuff;
    }
    #ifdef PATH_MAX
        cpath = sl_alloc_buffer(vm->arena, PATH_MAX + 1);
        realpath(path, cpath);
        return cpath;
    #else
        cpath = realpath(path, NULL);
        gcbuff = sl_alloc_buffer(vm->arena, strlen(cpath) + 1);
        strcpy(gcbuff, cpath);
        return gcbuff;
    #endif
}

int
sl_file_exists(sl_vm_t* vm, char* path)
{
    struct stat s;
    return !stat(sl_realpath(vm, path), &s);
}

int sl_abs_file_exists(char* path)
{
    struct stat s;
    if(path[0] != '/') {
        return 0;
    }
    return !stat(path, &s);
}

int sl_seed()
{
    struct timeval a;
    gettimeofday(&a, NULL);
    return (int)(a.tv_usec ^ a.tv_sec);
}
