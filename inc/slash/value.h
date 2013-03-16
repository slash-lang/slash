#ifndef SL_VALUE_H
#define SL_VALUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include "st.h"

struct sl_vm;

#define SL_MAX_INT (INT_MAX / 2)
#define SL_MIN_INT (INT_MIN / 2)

typedef union slval {
    intptr_t i;
}
SLVAL;

typedef union slid {
    size_t id;
}
SLID;

typedef enum sl_primitive_type {
    SL_T__INVALID = 0,
    SL_T_NIL,
    SL_T_FALSE,
    SL_T_TRUE,
    SL_T_CLASS,
    SL_T_OBJECT,
    SL_T_STRING,
    SL_T_INT,
    SL_T_FLOAT,
    SL_T_BIGNUM,
    SL_T_METHOD,
    SL_T_CACHED_METHOD_ENTRY,
    SL_T_BOUND_METHOD,
    SL_T_ARRAY,
    SL_T_DICT
}
sl_primitive_type_t;

typedef struct sl_object {
    sl_primitive_type_t primitive_type;
    SLVAL klass;
    sl_st_table_t* instance_variables;
}
sl_object_t;

typedef struct sl_class {
    sl_object_t base;
    SLID name;
    SLVAL super;
    SLVAL in;
    sl_st_table_t* constants;
    sl_st_table_t* class_variables;
    sl_st_table_t* instance_methods;
    sl_object_t*(*allocator)(struct sl_vm*);
    bool singleton;
}
sl_class_t;

typedef struct sl_string {
    sl_object_t base;
    uint8_t* buff;
    size_t buff_len;
    size_t char_len;
    int hash;
    int hash_set;
}
sl_string_t;

typedef struct sl_method {
    sl_object_t base;
    SLID name;
    SLVAL klass;
    char initialized;
    char is_c_func;
    int arity;
    union {
        struct {
            struct sl_vm_section* section;
            struct sl_vm_exec_ctx* parent_ctx;
        } sl;
        struct {
            SLVAL(*func)(/* vm, self, ... */);
        } c;
    } as;
}
sl_method_t;

typedef struct sl_cached_method_entry {
    /*
    be very careful with this struct.
    for the sake of memory efficiency, this doesn't have a full object header,
    so when pulling methods out of method tables, we need to do a primitive_type
    check first and follow the method pointer if it's a SL_T_CACHED_METHOD_ENTRY
    */
    sl_primitive_type_t primitive_type;
    uint32_t state;
    sl_method_t* method;
}
sl_cached_method_entry_t;

typedef struct sl_bound_method {
    sl_method_t method;
    SLVAL self;
}
sl_bound_method_t;

#define SL_IS_INT(val) ((val).i & 1)

int
sl_get_int(SLVAL val);

sl_object_t*
sl_get_ptr(SLVAL val);

SLVAL
sl_make_int(struct sl_vm* vm, long n);

SLVAL
sl_make_ptr(sl_object_t* ptr);

SLVAL
sl_expect(struct sl_vm* vm, SLVAL obj, SLVAL klass);

sl_primitive_type_t
sl_get_primitive_type(SLVAL val);

SLVAL
sl_allocate(struct sl_vm* vm, SLVAL klass);

int
sl_is_truthy(SLVAL val);

SLVAL
sl_make_bool(struct sl_vm* vm, bool b);

SLVAL
sl_not(struct sl_vm* vm, SLVAL val);

SLVAL
sl_to_bool(struct sl_vm* vm, SLVAL val);

int
sl_hash(struct sl_vm* vm, SLVAL val);

#endif
