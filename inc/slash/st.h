/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */
/* @(#) st.h 5.1 89/12/14 */

#ifndef ST_INCLUDED

#define ST_INCLUDED

#include "mem.h"

#if SIZEOF_LONG == SIZEOF_VOIDP
typedef unsigned long sl_st_data_t;
#elif SIZEOF_LONG_LONG == SIZEOF_VOIDP
typedef unsigned LONG_LONG sl_st_data_t;
#else
# error ---->> st.c requires sizeof(void*) == sizeof(long) to be compiled. <<---
-
#endif
#define ST_DATA_T_DEFINED

typedef struct sl_st_table sl_st_table_t;

struct sl_st_hash_type {
    int (*compare)();
    int (*hash)();
};

struct sl_st_table {
    sl_gc_arena_t* arena;
    struct sl_st_hash_type *type;
    int num_bins;
    int num_entries;
    struct sl_st_table_entry **bins;
};

typedef struct sl_st_table_entry sl_st_table_entry;

struct sl_st_table_entry
{
    unsigned int hash;
    sl_st_data_t key;
    sl_st_data_t record;
    sl_st_table_entry *next;
};

#define sl_st_is_member(table,key) sl_st_lookup(table,key,(sl_st_data_t *)0)

enum sl_st_retval {ST_CONTINUE, ST_STOP, ST_DELETE, ST_CHECK};

#ifndef _
# define _(args) args
#endif
#ifndef ANYARGS
# ifdef __cplusplus
#   define ANYARGS ...
# else
#   define ANYARGS
# endif
#endif

sl_st_table_t *sl_st_init_table _((sl_gc_arena_t* arena, struct sl_st_hash_type *));
sl_st_table_t *sl_st_init_table_with_size _((sl_gc_arena_t* arena, struct sl_st_hash_type *, int));
sl_st_table_t *sl_st_init_numtable _((sl_gc_arena_t* arena));
sl_st_table_t *sl_st_init_numtable_with_size _((sl_gc_arena_t* arena, int));
sl_st_table_t *sl_st_init_strtable _((sl_gc_arena_t* arena));
sl_st_table_t *sl_st_init_strtable_with_size _((sl_gc_arena_t* arena, int));
int sl_st_delete _((sl_st_table_t *, sl_st_data_t *, sl_st_data_t *));
int sl_st_delete_safe _((sl_st_table_t *, sl_st_data_t *, sl_st_data_t *, sl_st_data_t));
int sl_st_insert _((sl_st_table_t *, sl_st_data_t, sl_st_data_t));
int sl_st_lookup _((sl_st_table_t *, sl_st_data_t, sl_st_data_t *));
int sl_st_foreach _((sl_st_table_t *, int (*)(ANYARGS), sl_st_data_t));
void st_add_direct _((sl_st_table_t *, sl_st_data_t, sl_st_data_t));
void st_free_table _((sl_st_table_t *));
void st_cleanup_safe _((sl_st_table_t *, sl_st_data_t));
sl_st_table_t *st_copy _((sl_st_table_t *));

#define SL_ST_NUMCMP	((int (*)()) 0)
#define SL_ST_NUMHASH	((int (*)()) -2)

#define sl_st_numcmp	SL_ST_NUMCMP
#define sl_st_numhash	SL_ST_NUMHASH

int sl_st_strhash();

#endif /* ST_INCLUDED */
