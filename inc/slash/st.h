/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */
/* @(#) st.h 5.1 89/12/14 */

#ifndef SL_ST_INCLUDED

#define SL_ST_INCLUDED

typedef unsigned long sl_st_data_t;

typedef struct sl_st_table sl_st_table_t;

struct sl_st_hash_type {
    int (*compare)();
    int (*hash)();
};

struct sl_st_table {
    struct sl_vm* vm;
    struct sl_st_hash_type *type;
    int num_entries;
    int num_bins;
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

enum sl_st_retval {
    SL_ST_CONTINUE,
    SL_ST_STOP,
    SL_ST_DELETE,
    SL_ST_CHECK
};

sl_st_table_t*
sl_st_init_table(struct sl_vm* vm, struct sl_st_hash_type *);

sl_st_table_t*
sl_st_init_table_with_size(struct sl_vm* vm, struct sl_st_hash_type *, int);

sl_st_table_t*
sl_st_init_numtable(struct sl_vm* vm);

sl_st_table_t*
sl_st_init_numtable_with_size(struct sl_vm* vm, int);

int
sl_st_delete(sl_st_table_t *, sl_st_data_t *, sl_st_data_t *);

int
sl_st_delete_safe(sl_st_table_t *, sl_st_data_t *, sl_st_data_t *, sl_st_data_t);

int
sl_st_insert(sl_st_table_t *, sl_st_data_t, sl_st_data_t);

int
sl_st_lookup(sl_st_table_t *, sl_st_data_t, sl_st_data_t *);

int
sl_st_foreach(sl_st_table_t *, int (*)(), sl_st_data_t);

void
sl_st_add_direct(sl_st_table_t *, sl_st_data_t, sl_st_data_t);

void
sl_st_cleanup_safe(sl_st_table_t *, sl_st_data_t);

sl_st_table_t*
sl_st_copy(sl_st_table_t *);

#endif
