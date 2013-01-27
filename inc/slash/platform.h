#ifndef SL_PLATFORM_H
#define SL_PLATFORM_H

#include <setjmp.h>

struct sl_vm;

char*
sl_realpath(struct sl_vm* vm, char* path);

typedef enum sl_file_type {
    SL_FT_NO_EXIST,
    SL_FT_FILE,
    SL_FT_DIR
}
sl_file_type_t;

int
sl_file_exists(struct sl_vm* vm, char* path);

sl_file_type_t
sl_file_type(struct sl_vm* vm, char* path);

int
sl_abs_file_exists(char* path);

int
sl_seed();

void*
sl_stack_limit();

char**
sl_environ(struct sl_vm* vm);

#ifdef SL_HAS_BUILTIN_EXPECT
    #define sl_likely(x) __builtin_expect((x), 1)
    #define sl_unlikely(x) __builtin_expect((x), 0)
#else
    #define sl_likely(x) (x)
    #define sl_unlikely(x) (x)
#endif

#ifdef __APPLE__
    /* Mac OS X's setjmp() and longjmp() save the signal mask which is SLOOOOW.
       The _setjmp() and _longjmp() alternatives do not. */
    #define sl_setjmp(env) _setjmp(env)
    #define sl_longjmp(env, ret) _longjmp(env, ret)
#else
    #define sl_setjmp(env) setjmp(env)
    #define sl_longjmp(env, ret) longjmp(env, ret)
#endif

#ifdef _WIN32
    #include <malloc.h>
#else
    #include <alloca.h>
#endif

#include "vm.h"

#endif
