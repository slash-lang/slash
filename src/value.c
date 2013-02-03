#include <stdint.h>
#include <stdlib.h>
#include <slash/lib/bignum.h>
#include <slash/value.h>
#include <slash/vm.h>
#include <slash/st.h>
#include <slash/string.h>
#include <slash/class.h>
#include <slash/object.h>

int
sl_get_int(SLVAL val)
{
    return val.i >> 1;
}

sl_object_t* sl_get_ptr(SLVAL val)
{
    return (void*)val.i;
}

SLVAL
sl_make_int(sl_vm_t* vm, long n)
{
    SLVAL v;
    if(n > SL_MAX_INT || n < SL_MIN_INT) {
        return sl_make_bignum(vm, n);
    }
    v.i = (n << 1) | 1;
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
    SLVAL err;
    if(!sl_is_a(vm, obj, klass)) {
        err = sl_make_cstring(vm, "Expected ");
        err = sl_string_concat(vm, err, sl_inspect(vm, klass));
        err = sl_string_concat(vm, err, sl_make_cstring(vm, ", instead have "));
        err = sl_string_concat(vm, err, sl_inspect(vm, obj));
        sl_throw(vm, sl_make_error2(vm, vm->lib.TypeError, err));
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
    sl_expect(vm, vklass, vm->lib.Class);
    klass = (sl_class_t*)sl_get_ptr(vklass);
    if(klass->allocator) {
        obj = klass->allocator(vm);
        if(obj->primitive_type == SL_T__INVALID) {
            obj->primitive_type = SL_T_OBJECT;
        }
    } else {
        obj = sl_alloc(vm->arena, sizeof(sl_object_t));
        obj->primitive_type = SL_T_OBJECT;
    }
    obj->klass = vklass;
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

SLVAL
sl_make_bool(struct sl_vm* vm, bool b)
{
    if(b) {
        return vm->lib._true;
    } else {
        return vm->lib._false;
    }
}

int
sl_hash(sl_vm_t* vm, SLVAL val)
{
    SLVAL hash = sl_send_id(vm, val, vm->id.hash, 0);
    switch(sl_get_primitive_type(hash)) {
        case SL_T_INT:
            return sl_get_int(hash) ^ vm->hash_seed;
        case SL_T_BIGNUM:
            return sl_get_int(sl_bignum_hash(vm, hash)) ^ vm->hash_seed;
        default:
            sl_throw_message2(vm, vm->lib.TypeError, "Expected #hash method to return Int or Bignum");
            return 0; /* never reached */
    }
}
