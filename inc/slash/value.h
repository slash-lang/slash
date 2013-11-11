#ifndef SL_VALUE_H
#define SL_VALUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <gmp.h>
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
    SL_T__INVALID       = 0,
    SL_T_NIL            = 1,
    SL_T_FALSE          = 2,
    SL_T_TRUE           = 3,
    SL_T_CLASS          = 4,
    SL_T_OBJECT         = 5,
    SL_T_STRING         = 6,
    SL_T_INT            = 7,
    SL_T_FLOAT          = 8,
    SL_T_BIGNUM         = 9,
    SL_T_METHOD         = 10,
    SL_T_ARRAY          = 12,
    SL_T_DICT           = 13,
    SL_T_DATA           = 14,
}
sl_primitive_type_t;

typedef struct sl_object {
    unsigned primitive_type : 4;
    unsigned user_flags : 2;
    SLVAL klass;
    sl_st_table_t* instance_variables;
}
sl_object_t;

typedef struct sl_class {
    sl_object_t base;
    SLVAL super;
    sl_st_table_t* constants;
    sl_st_table_t* instance_methods;
    struct {
        sl_st_table_t* class_variables;
        SLVAL in;
        SLVAL doc;
        SLID name;
        sl_object_t*(*allocator)(struct sl_vm*);
    }* extra;
}
sl_class_t;
#define SL_FLAG_CLASS_SINGLETON (1 << 0)

typedef struct sl_string {
    sl_object_t base;
    uint8_t* buff;
    size_t buff_len;
    size_t char_len;
    int hash;
}
sl_string_t;
#define SL_FLAG_STRING_HASH_SET (1 << 0)

typedef struct sl_float {
    sl_object_t base;
    double d;
}
sl_float_t;

typedef struct {
    sl_object_t base;
    mpz_t mpz;
}
sl_bignum_t;

typedef struct sl_method {
    sl_object_t base;
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
    struct {
        SLID name;
        SLVAL klass;
        SLVAL doc;
        SLVAL bound_self;
    }* extra;
}
sl_method_t;
#define SL_FLAG_METHOD_INITIALIZED  (1 << 0)
#define SL_FLAG_METHOD_IS_C_FUNC    (1 << 1)

typedef struct {
    sl_object_t base;
    size_t count;
    size_t capacity;
    SLVAL* items;
}
sl_array_t;
#define SL_FLAG_ARRAY_ENUMERATOR_INSPECTING (1 << 0)

typedef struct {
    sl_object_t base;
    SLVAL* items;
    size_t count;
    size_t at;
}
sl_array_enumerator_t;

typedef struct {
    sl_object_t base;
    sl_st_table_t* st;
}
sl_dict_t;
#define SL_FLAG_DICT_INSPECTING (1 << 0)

typedef struct {
    sl_object_t base;
    SLVAL dict;
    SLVAL* keys;
    size_t count;
    size_t at;
}
sl_dict_enumerator_t;
#define SL_FLAG_DICT_ENUMERATOR_INITIALIZED (1 << 0)

typedef struct sl_data_type {
    const char* name;
    void(*free)(struct sl_vm*, void*);
}
sl_data_type_t;

typedef struct sl_data {
    sl_object_t base;
    struct sl_vm* vm; // hack - get rid of this
    sl_data_type_t* type;
    void* data;
}
sl_data_t;

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

SLVAL
sl_real_class(struct sl_vm* vm, SLVAL val);

#endif
