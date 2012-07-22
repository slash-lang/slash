#define _GNU_SOURCE
#include <pthread.h>
#include "platform.h"

void*
sl_stack_limit()
{
    pthread_t self = pthread_self();
    pthread_attr_t attr;
    void* addr;
    size_t size;
    pthread_getattr_np(self, &attr);
    pthread_attr_getstackaddr(&attr, &addr);
    pthread_attr_getstacksize(&attr, &size);
    return (void*)((size_t)addr - size + 65536);
}