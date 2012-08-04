#include <stdlib.h>
#include <string.h>
#include "mem.h"

typedef struct sl_gc_alloc {
    struct sl_gc_alloc* next;
    void* ptr;
    size_t size;
    char mark_flag;
    char scan_pointers;
    void(*finalizer)(void* ptr);
}
sl_gc_alloc_t;

struct sl_gc_arena {
    sl_gc_alloc_t** table;
    size_t table_count;
    intptr_t ptr_mask;
    size_t alloc_count;
    intptr_t stack_top;
    int mark_flag;
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
    arena->ptr_mask = 0xff;
    arena->table_count = 256;
    arena->table = calloc(arena->table_count, sizeof(sl_gc_alloc_t*));
    arena->alloc_count = 0;
    arena->mark_flag = 0;
    return arena;
}

void
sl_free_gc_arena(sl_gc_arena_t* arena)
{
    size_t i;
    sl_gc_alloc_t *alloc, *next;
    for(i = 0; i < arena->table_count; i++) {
        alloc = arena->table[i];
        while(alloc) {
            next = alloc->next;
            if(alloc->finalizer) {
                alloc->finalizer(alloc->ptr);
            }
            free(alloc->ptr);
            free(alloc);
            alloc = next;
        }
    }
    free(arena->table);
    free(arena);
}

static sl_gc_alloc_t*
sl_gc_find_alloc(sl_gc_arena_t* arena, void* ptr, sl_gc_alloc_t** prev)
{
    intptr_t hash = remove_insignificant_bits(ptr) & arena->ptr_mask;
    sl_gc_alloc_t *alloc = arena->table[hash];
    while(alloc) {
        if(alloc->ptr == ptr) {
            return alloc;
        }
        if(prev) {
            *prev = alloc;
        }
    }
    if(prev) {
        *prev = NULL;
    }
    return NULL;
}

void*
sl_alloc(sl_gc_arena_t* arena, size_t size)
{
    sl_gc_alloc_t* alloc = malloc(sizeof(sl_gc_alloc_t));
    void* ptr;
    intptr_t hash;
    
    /* align size: */
    size += sizeof(intptr_t) - 1;
    size &= ~(sizeof(intptr_t) - 1);
    
    ptr = malloc(size);
    memset(ptr, 0, size);
    hash = remove_insignificant_bits(ptr) & arena->ptr_mask;
    alloc->ptr = ptr;
    alloc->size = size;
    alloc->next = arena->table[hash];
    alloc->finalizer = NULL;
    alloc->mark_flag = arena->mark_flag;
    arena->table[hash] = alloc->next;
    arena->alloc_count++;
    return ptr;
}

void*
sl_alloc_buffer(sl_gc_arena_t* arena, size_t size)
{
    void* ptr = sl_alloc(arena, size);
    sl_gc_find_alloc(arena, ptr, NULL)->scan_pointers = 0;
    return ptr;
}

void*
sl_realloc(sl_gc_arena_t* arena, void* ptr, size_t new_size)
{
    if(ptr == NULL) {
        return sl_alloc(arena, new_size);
    }
    sl_gc_alloc_t* old_alloc = sl_gc_find_alloc(arena, ptr, NULL);
    void* new_ptr = sl_alloc(arena, new_size);
    sl_gc_alloc_t* new_alloc = sl_gc_find_alloc(arena, new_ptr, NULL);
    new_alloc->scan_pointers = old_alloc->scan_pointers;
    if(old_alloc->size < new_size) {
        new_size = old_alloc->size;
    }
    memcpy(new_ptr, ptr, new_size);
    return new_ptr;
}

static void
sl_gc_mark_allocation(sl_gc_arena_t* arena, sl_gc_alloc_t* alloc)
{
    intptr_t addr = (intptr_t)alloc->ptr;
    intptr_t max = addr + alloc->size;
    intptr_t ptr;
    if(alloc->mark_flag == arena->mark_flag) {
        return;
    }
    alloc->mark_flag = arena->mark_flag;
    if(!alloc->scan_pointers) {
        return;
    }
    while(addr < max) {
        ptr = *(intptr_t*)addr;
        if(ptr & (sizeof(intptr_t) - 1)) {
            /* if the pointer is not aligned, ignore it */
            continue;
        }
        alloc = sl_gc_find_alloc(arena, (void*)ptr, NULL);
        if(alloc) {
            sl_gc_mark_allocation(arena, alloc);
        }
        addr += sizeof(intptr_t);
    }
}

static void
sl_gc_mark_stack(sl_gc_arena_t* arena)
{
    intptr_t addr = arena->stack_top;
    sl_gc_alloc_t* alloc;
    intptr_t ptr;
    /* start at the top of the stack and work downwards, testing for pointers */
    while(addr > (intptr_t)&addr) {
        ptr = *(intptr_t*)addr;
        if(ptr & (sizeof(intptr_t) - 1)) {
            /* if the pointer is not aligned, ignore it */
            continue;
        }
        alloc = sl_gc_find_alloc(arena, (void*)ptr, NULL);
        if(alloc) {
            sl_gc_mark_allocation(arena, alloc);
        }
        /* only consider aligned addresses */
        addr -= sizeof(intptr_t);
    }
}

void
sl_gc_run(sl_gc_arena_t* arena)
{
    sl_gc_mark_stack(arena);
}

void
sl_gc_set_stack_top(sl_gc_arena_t* arena, void* ptr)
{
    arena->stack_top = (intptr_t)ptr;
}

void
sl_gc_set_finalizer(sl_gc_arena_t* arena, void* ptr, void(*finalizer)(void*))
{
    sl_gc_find_alloc(arena, ptr, NULL)->finalizer = finalizer;
}
