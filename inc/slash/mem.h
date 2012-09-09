#ifndef SL_MEM_H
#define SL_MEM_H

#include <stddef.h>

typedef struct sl_gc_arena
sl_gc_arena_t;

sl_gc_arena_t*
sl_make_gc_arena();

void*
sl_alloc(sl_gc_arena_t* arena, size_t size);

void*
sl_alloc_buffer(sl_gc_arena_t* arena, size_t size);

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
