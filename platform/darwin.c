#include <pthread.h>
#include <slash/platform.h>

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

char*** _NSGetEnviron();

char**
sl_environ(struct sl_vm* vm)
{
    return *_NSGetEnviron();
    (void)vm;
}
