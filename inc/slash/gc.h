#ifndef SL_GC_H
#define SL_GC_H

#include <stddef.h>

typedef struct sl_gc_arena
sl_gc_arena_t;

typedef struct sl_gc_shape {
    void(*mark)(sl_gc_arena_t*, void*);
    void(*finalize)(void*);
}
sl_gc_shape_t;

extern sl_gc_shape_t
sl_gc_pointer_free,
sl_gc_conservative;

sl_gc_arena_t*
sl_make_gc_arena();

void*
sl_alloc(sl_gc_arena_t* arena, size_t size);

void*
sl_alloc2(sl_gc_arena_t* arena, sl_gc_shape_t* shape, size_t size);

void*
sl_alloc_buffer(sl_gc_arena_t* arena, size_t size);

void*
sl_alloc_conservative(sl_gc_arena_t* arena, size_t size);

void*
sl_realloc(sl_gc_arena_t* arena, void* ptr, size_t new_size);

void
sl_gc_run(sl_gc_arena_t* arena);

void*
sl_gc_get_stack_top(sl_gc_arena_t* arena);

void
sl_gc_set_stack_top(sl_gc_arena_t* arena, void* ptr);

void
sl_gc_set_finalizer(void* ptr, void(*finalizer)(void*));

void
sl_free_gc_arena(sl_gc_arena_t* arena);

void
sl_gc_enable(sl_gc_arena_t* arena);

void
sl_gc_disable(sl_gc_arena_t* arena);

size_t
sl_gc_alloc_count(sl_gc_arena_t* arena);

size_t
sl_gc_memory_usage(sl_gc_arena_t* arena);

#endif
