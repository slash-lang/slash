#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <gc.h>
#include "lib/bignum.h"
#include "value.h"
#include "vm.h"
#include "st.h"
#include "string.h"
#include "class.h"

int
sl_get_int(SLVAL val)
{
    if(sl_get_primitive_type(val) != SL_T_INT) {
        /* throw type error... @TODO */
        abort();
    }
    return val.i / 2;
}

sl_object_t* sl_get_ptr(SLVAL val)
{
    return (void*)val.i;
}

SLVAL
sl_make_int(sl_vm_t* vm, int n)
{
    SLVAL v;
    if(n > INT_MAX / 2 || n < INT_MIN / 2) {
        return sl_make_bignum(vm, n);
    }
    v.i = n * 2 | 1;
    return v;
}

SLVAL
sl_make_ptr(sl_object_t* ptr)
{
    SLVAL v;
    v.i = (intptr_t)ptr;
    return v;
}

SLVAL
sl_expect(sl_vm_t* vm, SLVAL obj, SLVAL klass)
{
    if(!sl_is_a(vm, obj, klass)) {
        /* @TODO error */
        abort();
    }
    return obj;
}

sl_primitive_type_t
sl_get_primitive_type(SLVAL val)
{
    if(val.i & 1) {
        return SL_T_INT;
    } else {
        return sl_get_ptr(val)->primitive_type;
    }
}

SLVAL
sl_allocate(sl_vm_t* vm, SLVAL vklass)
{
    sl_class_t* klass;
    sl_object_t* obj;
    if(sl_get_primitive_type(vklass) != SL_T_CLASS) {
        /* throw type error... @TODO */
        (void)vm;
        abort();
    }
    klass = (sl_class_t*)sl_get_ptr(vklass);
    if(klass->allocator) {
        obj = klass->allocator();
        if(obj->primitive_type == SL_T__INVALID) {
            obj->primitive_type = SL_T_OBJECT;
        }
    } else {
        obj = GC_MALLOC(sizeof(sl_object_t));
        obj->primitive_type = SL_T_OBJECT;
    }
    obj->klass = vklass;
    obj->instance_variables = st_init_table(&sl_string_hash_type);
    return sl_make_ptr(obj);
}

int
sl_is_truthy(SLVAL val)
{
    sl_primitive_type_t type = sl_get_primitive_type(val);
    if(type == SL_T_NIL || type == SL_T_FALSE) {
        return 0;
    } else {
        return 1;
    }
}
