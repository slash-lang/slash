#ifndef SL_MEM_H
#define SL_MEM_H

#include <stddef.h>

typedef struct sl_gc_arena
sl_gc_arena_t;

sl_gc_arena_t*
sl_make_gc_arena();

/*#define SL_LOG_ALLOCS*/
#ifdef SL_LOG_ALLOCS
    #define SL_ALLOC_ADDITIONAL_ARGS , int line, const char* file
    #define sl_alloc(arena,size) sl_alloc_((arena),(size),__LINE__,__FILE__)
    #define sl_alloc_buffer(arena,size) sl_alloc_buffer_((arena),(size),__LINE__,__FILE__)
#else
    #define SL_ALLOC_ADDITIONAL_ARGS
    #define sl_alloc(arena,size) sl_alloc_((arena),(size))
    #define sl_alloc_buffer(arena,size) sl_alloc_buffer_((arena),(size))
#endif

void*
sl_alloc_(sl_gc_arena_t* arena, size_t size SL_ALLOC_ADDITIONAL_ARGS);

void*
sl_alloc_buffer_(sl_gc_arena_t* arena, size_t size SL_ALLOC_ADDITIONAL_ARGS);

void*
sl_realloc(sl_gc_arena_t* arena, void* ptr, size_t new_size);

void
sl_gc_run(sl_gc_arena_t* arena);

void
sl_gc_set_stack_top(sl_gc_arena_t* arena, void* ptr);

void
sl_gc_set_finalizer(sl_gc_arena_t* arena, void* ptr, void(*finalizer)(void*));

void
sl_free_gc_arena(sl_gc_arena_t* arena);

#endif
