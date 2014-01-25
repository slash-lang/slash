#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <slash/gc.h>
#include <slash/platform.h>
#include <stdint.h>

#define POINTER_ALIGN_BYTES (4)
#define ALLOCS_PER_GC_RUN (10000)
#define ALLOC_BUCKET_COUNT (65536)

typedef struct sl_gc_alloc {
    struct sl_gc_alloc* next;
    size_t size;
    sl_gc_shape_t* shape;
    char mark_flag;
}
sl_gc_alloc_t;

static void*
ptr_for_alloc(sl_gc_alloc_t* alloc)
{
    return alloc + 1;
}

static sl_gc_alloc_t*
alloc_for_ptr(void* ptr)
{
    return (sl_gc_alloc_t*)ptr - 1;
}

static void
free_alloc(sl_gc_alloc_t* alloc)
{
    if(alloc->shape->finalize) {
        alloc->shape->finalize(ptr_for_alloc(alloc));
    }
    free(alloc);
}

typedef struct sl_gc_root {
    struct sl_gc_root* next;
    void* addr;
    size_t size;
}
sl_gc_root_t;

struct sl_gc_arena {
    sl_gc_alloc_t** table;
    size_t table_count;
    size_t pointer_mask;
    size_t alloc_count;
    size_t memory_usage;
    size_t allocs_since_gc;
    intptr_t stack_top;
    int mark_flag;
    int enabled;
};

static intptr_t
remove_insignificant_bits(void* ptr)
{
    intptr_t i = (intptr_t)ptr;
    size_t sz = sizeof(void*);
    while(sz > 1) {
        sz /= 2;
        i >>= 1;
    }
    return i;
}

sl_gc_arena_t*
sl_make_gc_arena()
{
    sl_gc_arena_t* arena = malloc(sizeof(sl_gc_arena_t));
    arena->table_count = ALLOC_BUCKET_COUNT;
    arena->pointer_mask = arena->table_count - 1;
    arena->table = calloc(arena->table_count, sizeof(sl_gc_alloc_t*));
    arena->alloc_count = 0;
    arena->memory_usage = sizeof(*arena) + (arena->table_count * sizeof(sl_gc_alloc_t*));
    arena->allocs_since_gc = 0;
    arena->mark_flag = 0;
    arena->enabled = 1;
    return arena;
}

void
sl_free_gc_arena(sl_gc_arena_t* arena)
{
    size_t i;

    for(i = 0; i < arena->table_count; i++) {
        sl_gc_alloc_t *alloc, *next;
        alloc = arena->table[i];
        while(alloc) {
            next = alloc->next;
            free_alloc(alloc);
            alloc = next;
        }
    }

    free(arena->table);
    free(arena);
}

static sl_gc_alloc_t*
sl_gc_find_alloc(sl_gc_arena_t* arena, void* ptr)
{
    intptr_t hash = remove_insignificant_bits(ptr) & arena->pointer_mask;
    sl_gc_alloc_t *alloc = arena->table[hash];
    while(alloc) {
        if(ptr_for_alloc(alloc) == ptr) {
            return alloc;
        }
        alloc = alloc->next;
    }
    return NULL;
}

void*
sl_alloc(sl_gc_arena_t* arena, size_t size)
{
    return sl_alloc2(arena, &sl_gc_conservative, size);
}

void*
sl_alloc2(sl_gc_arena_t* arena, sl_gc_shape_t* shape, size_t size)
{
    sl_gc_alloc_t* alloc = malloc(sizeof(sl_gc_alloc_t) + size);
    arena->memory_usage += sizeof(sizeof(sl_gc_alloc_t)) + size;
    void* ptr;
    intptr_t hash;

    if(arena->allocs_since_gc > ALLOCS_PER_GC_RUN) {
        sl_gc_run(arena);
    }

    ptr = ptr_for_alloc(alloc);
    memset(ptr, 0, size);
    hash = remove_insignificant_bits(ptr) & arena->pointer_mask;
    alloc->size = size;
    alloc->next = arena->table[hash];
    alloc->mark_flag = arena->mark_flag;
    alloc->shape = shape;
    arena->table[hash] = alloc;
    arena->alloc_count++;
    arena->allocs_since_gc++;
    return ptr;
}

void*
sl_alloc_buffer(sl_gc_arena_t* arena, size_t size)
{
    return sl_alloc2(arena, &sl_gc_pointer_free, size);
}

void*
sl_realloc(sl_gc_arena_t* arena, void* ptr, size_t new_size)
{
    /* @TODO: more more efficient */
    sl_gc_alloc_t* old_alloc = alloc_for_ptr(ptr);
    void* new_ptr = sl_alloc2(arena, old_alloc->shape, new_size);
    if(old_alloc->size < new_size) {
        new_size = old_alloc->size;
    }
    memcpy(new_ptr, ptr, new_size);
    return new_ptr;
}

static void
sl_gc_mark_allocation(sl_gc_arena_t* arena, sl_gc_alloc_t* alloc);

static void
sl_gc_mark_region(sl_gc_arena_t* arena, void* ptr, size_t size)
{
    intptr_t addr = (intptr_t)ptr;
    intptr_t max = addr + size;
    for(; addr + (intptr_t)sizeof(void*) <= max; addr += POINTER_ALIGN_BYTES) {
        intptr_t ptr = *(intptr_t*)addr;
        if(ptr & (POINTER_ALIGN_BYTES - 1)) {
            /* if the pointer is not aligned, ignore it */
            continue;
        }
        sl_gc_alloc_t* alloc = sl_gc_find_alloc(arena, (void*)ptr);
        if(alloc) {
            sl_gc_mark_allocation(arena, alloc);
        }
    }
}

static void
sl_gc_mark_allocation(sl_gc_arena_t* arena, sl_gc_alloc_t* alloc)
{
    if(alloc->mark_flag == arena->mark_flag) {
        return;
    }

    alloc->mark_flag = arena->mark_flag;

    if(alloc->shape->mark) {
        alloc->shape->mark(arena, ptr_for_alloc(alloc));
    }
}

static void
sl_gc_mark_stack(sl_gc_arena_t* arena)
{
    intptr_t addr;
    sl_gc_alloc_t* alloc;
    intptr_t ptr;
    intptr_t stack_bottom = (intptr_t)&addr;
    /* start at the top of the stack and work downwards, testing for pointers */
    for(addr = arena->stack_top; addr > stack_bottom; addr -= POINTER_ALIGN_BYTES) {
        ptr = *(intptr_t*)addr;
        if(ptr & (POINTER_ALIGN_BYTES - 1)) {
            /* if the pointer is not aligned, ignore it */
            continue;
        }
        alloc = sl_gc_find_alloc(arena, (void*)ptr);
        if(alloc) {
            sl_gc_mark_allocation(arena, alloc);
        }
    }
}

static void
sweep_bucket(sl_gc_arena_t* arena, int i)
{
    sl_gc_alloc_t* alloc = arena->table[i];
    sl_gc_alloc_t* prev = NULL;

    while(alloc) {
        sl_gc_alloc_t* next = alloc->next;

        if(alloc->mark_flag != arena->mark_flag) {
            free_alloc(alloc);

            if(prev) {
                prev->next = next;
            } else {
                arena->table[i] = next;
            }
        } else {
            prev = alloc;
        }

        alloc = next;
    }
}

static void
sl_gc_sweep(sl_gc_arena_t* arena)
{
    for(size_t i = 0; i < arena->table_count; i++) {
        sweep_bucket(arena, i);
    }
}

void
sl_gc_run(sl_gc_arena_t* arena)
{
    if(!arena->enabled) {
        arena->allocs_since_gc = 0;
        return;
    }

    jmp_buf regs;
    setjmp(regs);

    arena->allocs_since_gc = 0;
    arena->mark_flag = !arena->mark_flag;
    sl_gc_mark_region(arena, &regs, sizeof(regs));
    sl_gc_mark_stack(arena);
    sl_gc_sweep(arena);
}

void*
sl_gc_get_stack_top(sl_gc_arena_t* arena)
{
    return (void*)arena->stack_top;
}

void
sl_gc_set_stack_top(sl_gc_arena_t* arena, void* ptr)
{
    arena->stack_top = (intptr_t)ptr & ~(POINTER_ALIGN_BYTES - 1);
}

void
sl_gc_enable(sl_gc_arena_t* arena)
{
    arena->enabled = 1;
}

void
sl_gc_disable(sl_gc_arena_t* arena)
{
    arena->enabled = 0;
}

size_t
sl_gc_alloc_count(sl_gc_arena_t* arena)
{
    return arena->alloc_count;
}

size_t
sl_gc_memory_usage(sl_gc_arena_t* arena)
{
    return arena->memory_usage;
}

void
sl_gc_conservative_mark(sl_gc_arena_t* arena, void* ptr)
{
    sl_gc_alloc_t* alloc = alloc_for_ptr(ptr);
    sl_gc_mark_region(arena, ptr, alloc->size);
}

sl_gc_shape_t
sl_gc_conservative = {
    .mark     = sl_gc_conservative_mark,
    .finalize = NULL,
};

sl_gc_shape_t
sl_gc_pointer_free = {
    .mark     = NULL,
    .finalize = NULL,
};
