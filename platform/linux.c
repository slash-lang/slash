#define _GNU_SOURCE
#include <pthread.h>
#include <slash/platform.h>

/*
 * On Linux, the pthread functions sometimes give an unreasonable size for the
 * stack. We'll set a hardcoded limit of 4 MB for the Slash stack and figure
 * out where the top of the stack is with the pthread functions.
 */
#define STACK_SIZE (4*1024*1024)

void*
sl_stack_limit()
{
    pthread_t self = pthread_self();
    pthread_attr_t attr;
    void* addr;
    size_t size;
    pthread_getattr_np(self, &attr);
    pthread_attr_getstack(&attr, &addr, &size);
    pthread_attr_destroy(&attr);
    size_t stack_top = (size_t)addr + size;
    return (void*)(stack_top - STACK_SIZE);
}
