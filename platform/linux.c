#define _GNU_SOURCE
#include <pthread.h>
#include "slash/platform.h"

void*
sl_stack_limit()
{
    pthread_t self = pthread_self();
    pthread_attr_t attr;
    void* addr;
    size_t size;
    pthread_getattr_np(self, &attr);
    pthread_attr_getstack(&attr, &addr, &size);
    return (void*)((size_t)addr - size + 65536);
}
