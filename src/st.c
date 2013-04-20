/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */

/* static	char	sccsid[] = "@(#) st.c 5.1 89/12/14 Crucible"; */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <slash/st.h>
#include <slash/mem.h>
#include <slash/vm.h>

#define ST_DEFAULT_MAX_DENSITY 5
#define ST_DEFAULT_INIT_TABLE_SIZE 11

/*
 * DEFAULT_MAX_DENSITY is the default for the largest we allow the
 * average number of items per bin before increasing the number of
 * bins
 *
 * DEFAULT_INIT_TABLE_SIZE is the default for the number of bins
 * allocated initially
 *
 */
static int numcmp(long, long);
static int numhash(long);
static struct sl_st_hash_type type_numhash =
{
    numcmp,
    numhash,
};

static int strhash(const char *);
static struct sl_st_hash_type type_strhash =
{
    strcmp,
    strhash,
};

static void rehash(sl_st_table_t *);

#define alloc(type) (type*)sl_alloc(tbl->vm->arena, sizeof(type))
#define Calloc(n,s) (char*)sl_alloc(tbl->vm->arena, (n) * (s))

#define EQUAL(table,x,y) ((x)==(y) || (*table->type->compare)((x),(y)) == 0)

#define do_hash(key,table) (unsigned int)(*(table)->type->hash)((key))
#define do_hash_bin(key,table) (do_hash(key, table)%(table)->num_bins)

/*
 * MINSIZE is the minimum size of a dictionary.
 */

#define MINSIZE 8

/*
Table of prime numbers 2^n+a, 2<=n<=30.
*/
static long primes[] =
{
    8 + 3,
    16 + 3,
    32 + 5,
    64 + 3,
    128 + 3,
    256 + 27,
    512 + 9,
    1024 + 9,
    2048 + 5,
    4096 + 3,
    8192 + 27,
    16384 + 43,
    32768 + 3,
    65536 + 45,
    131072 + 29,
    262144 + 3,
    524288 + 21,
    1048576 + 7,
    2097152 + 17,
    4194304 + 15,
    8388608 + 9,
    16777216 + 43,
    33554432 + 35,
    67108864 + 15,
    134217728 + 29,
    268435456 + 3,
    536870912 + 11,
    1073741824 + 85,
    0
};

static int
new_size(size)
unsigned int size;
{
    unsigned int i;

    unsigned int newsize;

    for (i = 0, newsize = MINSIZE;
        i < sizeof(primes)/sizeof(primes[0]);
    i++, newsize <<= 1) {
        if (newsize > size) return primes[i];
    }
    return -1;                                    /* should raise exception */
}

#ifdef HASH_LOG
static int collision = 0;
static int init_st = 0;

static void
stat_col()
{
    FILE *f = fopen("/tmp/col", "w");
    fprintf(f, "collision: %d\n", collision);
    fclose(f);
}
#endif

sl_st_table_t*
sl_st_init_table_with_size(sl_vm_t* vm, struct sl_st_hash_type* type, int size)
{
    sl_st_table_t *tbl;

    #ifdef HASH_LOG
    if (init_st == 0) {
        init_st = 1;
        atexit(stat_col);
    }
    #endif

    size = new_size(size);                        /* round up to prime number */

    tbl = sl_alloc(vm->arena, sizeof(sl_st_table_t));
    tbl->vm = vm;
    tbl->type = type;
    tbl->num_entries = 0;
    tbl->num_bins = size;
    tbl->bins = (sl_st_table_entry **)Calloc(size, sizeof(sl_st_table_entry*));

    return tbl;
}

sl_st_table_t*
sl_st_init_table(sl_vm_t* vm, struct sl_st_hash_type* type)
{
    return sl_st_init_table_with_size(vm, type, 0);
}

sl_st_table_t*
sl_st_init_numtable(sl_vm_t* vm)
{
    return sl_st_init_table(vm, &type_numhash);
}

sl_st_table_t*
sl_st_init_numtable_with_size(sl_vm_t* vm, int size)
{
    return sl_st_init_table_with_size(vm, &type_numhash, size);
}

sl_st_table_t*
sl_st_init_strtable(sl_vm_t* vm)
{
    return sl_st_init_table(vm, &type_strhash);
}

sl_st_table_t*
sl_st_init_strtable_with_size(sl_vm_t* vm, int size)
{
    return sl_st_init_table_with_size(vm, &type_strhash, size);
}

#define PTR_NOT_EQUAL(table, ptr, hash_val, key) \
    ((ptr) != 0 && (ptr->hash != (hash_val) || !EQUAL((table), (key), (ptr)->key)))

#ifdef HASH_LOG
#define COLLISION collision++
#else
#define COLLISION
#endif

#define FIND_ENTRY(table, ptr, hash_val, bin_pos) do {\
        bin_pos = hash_val%(table)->num_bins;\
        ptr = (table)->bins[bin_pos];\
        if (PTR_NOT_EQUAL(table, ptr, hash_val, key)) \
        { \
            COLLISION;\
            while (PTR_NOT_EQUAL(table, ptr->next, hash_val, key)) \
            { \
                ptr = ptr->next;\
            }\
            ptr = ptr->next;\
        }\
    } while (0)

        int
        sl_st_lookup(table, key, value)
        sl_st_table_t *table;
register sl_st_data_t key;
sl_st_data_t *value;
{
    unsigned int hash_val, bin_pos;
    register sl_st_table_entry *ptr;

    hash_val = do_hash(key, table);
    FIND_ENTRY(table, ptr, hash_val, bin_pos);

    if (ptr == 0) {
        return 0;
    }
    else {
        if (value != 0)  *value = ptr->record;
        return 1;
    }
}

#define ADD_DIRECT(table, key, value, hash_val, bin_pos)\
    do \
    { \
        sl_st_table_entry *entry;\
        if (table->num_entries/(table->num_bins) > ST_DEFAULT_MAX_DENSITY) \
        { \
            rehash(table);\
            bin_pos = hash_val % table->num_bins;\
        }\
    entry = alloc(sl_st_table_entry);\
    entry->hash = hash_val;\
    entry->key = key;\
    entry->record = value;\
    entry->next = table->bins[bin_pos];\
    table->bins[bin_pos] = entry;\
    table->num_entries++;\
} while (0)

int
sl_st_insert(tbl, key, value)
register sl_st_table_t *tbl;
register sl_st_data_t key;
sl_st_data_t value;
{
    unsigned int hash_val, bin_pos;
    register sl_st_table_entry *ptr;

    hash_val = do_hash(key, tbl);
    FIND_ENTRY(tbl, ptr, hash_val, bin_pos);

    if (ptr == 0) {
        ADD_DIRECT(tbl, key, value, hash_val, bin_pos);
        return 0;
    }
    else {
        ptr->record = value;
        return 1;
    }
}

void
sl_st_add_direct(tbl, key, value)
sl_st_table_t *tbl;
sl_st_data_t key;
sl_st_data_t value;
{
    unsigned int hash_val, bin_pos;

    hash_val = do_hash(key, tbl);
    bin_pos = hash_val % tbl->num_bins;
    ADD_DIRECT(tbl, key, value, hash_val, bin_pos);
}

static void
rehash(tbl)
register sl_st_table_t *tbl;
{
    register sl_st_table_entry *ptr, *next, **new_bins;
    int i, old_num_bins = tbl->num_bins, new_num_bins;
    unsigned int hash_val;

    new_num_bins = new_size(old_num_bins+1);
    new_bins = (sl_st_table_entry**)Calloc(new_num_bins, sizeof(sl_st_table_entry*));

    for(i = 0; i < old_num_bins; i++) {
        ptr = tbl->bins[i];
        while (ptr != 0) {
            next = ptr->next;
            hash_val = ptr->hash % new_num_bins;
            ptr->next = new_bins[hash_val];
            new_bins[hash_val] = ptr;
            ptr = next;
        }
    }
    tbl->num_bins = new_num_bins;
    tbl->bins = new_bins;
}

sl_st_table_t*
sl_st_copy(old_table)
sl_st_table_t *old_table;
{
    sl_st_table_t *new_table;
    sl_st_table_entry *ptr, *entry;
    int i, num_bins = old_table->num_bins;
    
    /*  this is only for alloc()'s expectation that there is a tbl local.
        it requires a tbl local to exist so it can locate the arena to use */
    sl_st_table_t* tbl = old_table;

    new_table = alloc(sl_st_table_t);
    if (new_table == 0) {
        return 0;
    }

    *new_table = *old_table;
    new_table->bins = (sl_st_table_entry**)
    Calloc((unsigned)num_bins, sizeof(sl_st_table_entry*));

    if (new_table->bins == 0) {
        return 0;
    }

    for(i = 0; i < num_bins; i++) {
        new_table->bins[i] = 0;
        ptr = old_table->bins[i];
        while (ptr != 0) {
            entry = alloc(sl_st_table_entry);
            if (entry == 0) {
                return 0;
            }
            *entry = *ptr;
            entry->next = new_table->bins[i];
            new_table->bins[i] = entry;
            ptr = ptr->next;
        }
    }
    return new_table;
}

int
sl_st_delete(table, key, value)
register sl_st_table_t *table;
register sl_st_data_t *key;
sl_st_data_t *value;
{
    unsigned int hash_val;
    sl_st_table_entry *tmp;
    register sl_st_table_entry *ptr;

    hash_val = do_hash_bin(*key, table);
    ptr = table->bins[hash_val];

    if (ptr == 0) {
        if (value != 0) *value = 0;
        return 0;
    }

    if (EQUAL(table, *key, ptr->key)) {
        table->bins[hash_val] = ptr->next;
        table->num_entries--;
        if (value != 0) *value = ptr->record;
        *key = ptr->key;
        return 1;
    }

    for(; ptr->next != 0; ptr = ptr->next) {
        if (EQUAL(table, ptr->next->key, *key)) {
            tmp = ptr->next;
            ptr->next = ptr->next->next;
            table->num_entries--;
            if (value != 0) *value = tmp->record;
            *key = tmp->key;
            return 1;
        }
    }

    return 0;
}

int
sl_st_delete_safe(table, key, value, never)
register sl_st_table_t *table;
register sl_st_data_t *key;
sl_st_data_t *value;
sl_st_data_t never;
{
    unsigned int hash_val;
    register sl_st_table_entry *ptr;

    hash_val = do_hash_bin(*key, table);
    ptr = table->bins[hash_val];

    if (ptr == 0) {
        if (value != 0) *value = 0;
        return 0;
    }

    for(; ptr != 0; ptr = ptr->next) {
        if ((ptr->key != never) && EQUAL(table, ptr->key, *key)) {
            table->num_entries--;
            *key = ptr->key;
            if (value != 0) *value = ptr->record;
            ptr->key = ptr->record = never;
            return 1;
        }
    }

    return 0;
}

static int
delete_never(sl_vm_t* vm, sl_st_data_t key, sl_st_data_t value, sl_st_data_t never)
{
    (void)key;
    (void)vm;
    if (value == never) return SL_ST_DELETE;
    return SL_ST_CONTINUE;
}

void
sl_st_cleanup_safe(table, never)
sl_st_table_t *table;
sl_st_data_t never;
{
    int num_entries = table->num_entries;

    sl_st_foreach(table, delete_never, never);
    table->num_entries = num_entries;
}

int
sl_st_foreach(sl_st_table_t* table, int (*func)(), sl_st_data_t arg)
{
    sl_st_table_entry *ptr, *last, *tmp;
    enum sl_st_retval retval;
    int i;

    for(i = 0; i < table->num_bins; i++) {
        last = 0;
        for(ptr = table->bins[i]; ptr != 0;) {
            retval = (*func)(table->vm, ptr->key, ptr->record, arg);
            switch (retval) {
                case SL_ST_CHECK:                    /* check if hash is modified during iteration */
                    tmp = 0;
                    if (i < table->num_bins) {
                        for (tmp = table->bins[i]; tmp; tmp=tmp->next) {
                            if (tmp == ptr) break;
                        }
                    }
                    if (!tmp) {
                        /* call func with error notice */
                        return 1;
                    }
                    /* fall through */
                case SL_ST_CONTINUE:
                    last = ptr;
                    ptr = ptr->next;
                    break;
                case SL_ST_STOP:
                    return 0;
                case SL_ST_DELETE:
                    tmp = ptr;
                    if (last == 0) {
                        table->bins[i] = ptr->next;
                    }
                    else {
                        last->next = ptr->next;
                    }
                    ptr = ptr->next;
                    table->num_entries--;
            }
        }
    }
    return 0;
}

static int
strhash(string)
register const char *string;
{
    register int c;

    #ifdef HASH_ELFHASH
    register unsigned int h = 0, g;

    while ((c = *string++) != '\0') {
        h = ( h << 4 ) + c;
        if ( g = h & 0xF0000000 )
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
    #elif defined(HASH_PERL)
    register int val = 0;

    while ((c = *string++) != '\0') {
        val += c;
        val += (val << 10);
        val ^= (val >> 6);
    }
    val += (val << 3);
    val ^= (val >> 11);

    return val + (val << 15);
    #else
    register int val = 0;

    while ((c = *string++) != '\0') {
        val = val*997 + c;
    }

    return val + (val>>5);
    #endif
}

static int
numcmp(x, y)
long x, y;
{
    return x != y;
}

static int
numhash(n)
long n;
{
    return n;
}
