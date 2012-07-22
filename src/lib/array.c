#include <stdlib.h>
#include <string.h>
#include <gc.h>
#include "slash.h"

typedef struct {
    sl_object_t base;
    size_t count;
    size_t capacity;
    SLVAL* items;
}
sl_array_t;

typedef struct {
    sl_object_t base;
    SLVAL* items;
    size_t count;
    size_t at;
}
sl_array_enumerator_t;

static sl_object_t*
allocate_array(sl_vm_t* vm)
{
    size_t i;
    sl_array_t* ary = GC_MALLOC(sizeof(sl_array_t));
    ary->capacity = 2;
    ary->count = 0;
    ary->items = GC_MALLOC(sizeof(SLVAL) * ary->capacity);
    for(i = 0; i < ary->capacity; i++) {
        ary->items[i] = vm->lib.nil;
    }
    return (sl_object_t*)ary;
}

static sl_object_t*
allocate_array_enumerator()
{
    return GC_MALLOC(sizeof(sl_array_enumerator_t));
}

static sl_array_t*
get_array(sl_vm_t* vm, SLVAL array)
{
    sl_expect(vm, array, vm->lib.Array);
    return (sl_array_t*)sl_get_ptr(array);
}

static void
array_resize(sl_vm_t* vm, sl_array_t* aryp, size_t new_size)
{
    size_t i, old_capacity;
    if(new_size == 0) {
        return;
    }
    if(new_size > aryp->capacity) {
        old_capacity = aryp->capacity;
        while(new_size > aryp->capacity) {
            aryp->capacity *= 2;
        }
        aryp->items = GC_REALLOC(aryp->items, sizeof(SLVAL) * aryp->capacity);
        for(i = old_capacity; i < aryp->capacity; i++) {
            aryp->items[i] = vm->lib.nil;
        }
    } else if(new_size < aryp->capacity / 2) {
        while(new_size < aryp->capacity / 2) {
            aryp->capacity /= 2;
        }
        aryp->items = GC_REALLOC(aryp->items, sizeof(SLVAL) * aryp->capacity);
    }
    aryp->count = new_size;
}

static SLVAL
sl_array_init(sl_vm_t* vm, SLVAL array, size_t count, SLVAL* items)
{
    sl_array_t* aryp = get_array(vm, array);
    aryp->count = count;
    aryp->capacity = count > 2 ? count : 2;
    aryp->items = GC_MALLOC(sizeof(SLVAL) * aryp->capacity);
    aryp->items[0] = vm->lib.nil;
    aryp->items[1] = vm->lib.nil;
    memcpy(aryp->items, items, sizeof(SLVAL) * count);
    return vm->lib.nil;
}

static SLVAL
sl_array_to_s(sl_vm_t* vm, SLVAL array)
{
    sl_array_t* aryp = get_array(vm, array);
    size_t i;
    SLVAL str = sl_make_cstring(vm, "[");
    for(i = 0; i < aryp->count; i++) {
        if(i != 0) {
            str = sl_string_concat(vm, str, sl_make_cstring(vm, ", "));
        }
        str = sl_string_concat(vm, str, sl_inspect(vm, aryp->items[i]));
    }    
    str = sl_string_concat(vm, str, sl_make_cstring(vm, "]"));
    return str;
}

static SLVAL
sl_array_enumerate(sl_vm_t* vm, SLVAL array)
{
    return sl_new(vm, vm->lib.Array_Enumerator, 1, &array);
}

static SLVAL
sl_array_enumerator_init(sl_vm_t* vm, SLVAL self, SLVAL array)
{
    sl_array_enumerator_t* e = (sl_array_enumerator_t*)sl_get_ptr(self);
    sl_array_t* a = get_array(vm, array);
    e->items = a->items;
    e->at = 0;
    e->count = a->count;
    return vm->lib.nil;
}

static SLVAL
sl_array_enumerator_next(sl_vm_t* vm, SLVAL self)
{
    sl_array_enumerator_t* e = (sl_array_enumerator_t*)sl_get_ptr(self);
    if(!e->items) {
        sl_throw_message2(vm, vm->lib.Error, "Invalid operator on Array::Enumerator");
    }
    if(++e->at > e->count) {
        return vm->lib._false;
    } else {
        return vm->lib._true;
    }
}

static SLVAL
sl_array_enumerator_current(sl_vm_t* vm, SLVAL self)
{
    sl_array_enumerator_t* e = (sl_array_enumerator_t*)sl_get_ptr(self);
    if(!e->items) {
        sl_throw_message2(vm, vm->lib.Error, "Invalid operator on Array::Enumerator");
    }
    if(e->at == 0 || e->at > e->count) {
        sl_throw_message2(vm, vm->lib.Error, "Invalid operator on Array::Enumerator");
    }
    return e->items[e->at - 1];
}

void
sl_init_array(sl_vm_t* vm)
{
    vm->lib.Array = sl_define_class(vm, "Array", vm->lib.Enumerable);
    sl_class_set_allocator(vm, vm->lib.Array, allocate_array);
    sl_define_method(vm, vm->lib.Array, "init", -1, sl_array_init);
    sl_define_method(vm, vm->lib.Array, "enumerate", 0, sl_array_enumerate);
    sl_define_method(vm, vm->lib.Array, "[]", 1, sl_array_get2);
    sl_define_method(vm, vm->lib.Array, "[]=", 2, sl_array_set2);
    sl_define_method(vm, vm->lib.Array, "length", 0, sl_array_length);
    sl_define_method(vm, vm->lib.Array, "push", -2, sl_array_push);
    sl_define_method(vm, vm->lib.Array, "pop", 0, sl_array_pop);
    sl_define_method(vm, vm->lib.Array, "unshift", -2, sl_array_unshift);
    sl_define_method(vm, vm->lib.Array, "shift", 0, sl_array_shift);
    sl_define_method(vm, vm->lib.Array, "to_s", 0, sl_array_to_s);
    sl_define_method(vm, vm->lib.Array, "inspect", 0, sl_array_to_s);
    
    vm->lib.Array_Enumerator = sl_define_class3(
        vm, sl_make_cstring(vm, "Enumerator"), vm->lib.Object, vm->lib.Array);
        
    sl_class_set_allocator(vm, vm->lib.Array_Enumerator, allocate_array_enumerator);
    sl_define_method(vm, vm->lib.Array_Enumerator, "init", 1, sl_array_enumerator_init);
    sl_define_method(vm, vm->lib.Array_Enumerator, "next", 0, sl_array_enumerator_next);
    sl_define_method(vm, vm->lib.Array_Enumerator, "current", 0, sl_array_enumerator_current);
}

SLVAL
sl_make_array(sl_vm_t* vm, size_t count, SLVAL* items)
{
    return sl_new(vm, vm->lib.Array, count, items);
}

SLVAL
sl_array_get(sl_vm_t* vm, SLVAL array, int i)
{
    return sl_array_get2(vm, array, sl_make_int(vm, i));
}

SLVAL
sl_array_get2(sl_vm_t* vm, SLVAL array, SLVAL vi)
{
    int i = sl_get_int(sl_expect(vm, vi, vm->lib.Int));
    sl_array_t* aryp = get_array(vm, array);
    if(i < 0) {
        i += aryp->count;
    }
    if(i < 0 || (size_t)i >= aryp->count) {
        return vm->lib.nil;
    }
    return aryp->items[i];
}

SLVAL
sl_array_set(sl_vm_t* vm, SLVAL array, int i, SLVAL val)
{
    return sl_array_set2(vm, array, sl_make_int(vm, i), val);
}

SLVAL
sl_array_set2(sl_vm_t* vm, SLVAL array, SLVAL vi, SLVAL val)
{
    int i = sl_get_int(sl_expect(vm, vi, vm->lib.Int));
    sl_array_t* aryp = get_array(vm, array);
    if(i < 0) {
        i += aryp->count;
    }
    if(i < 0) {
        sl_throw_message2(vm, vm->lib.ArgumentError, "index too small");
    }
    if((size_t)i >= aryp->capacity) {
        array_resize(vm, aryp, (size_t)i + 1);
    }
    return aryp->items[i] = val;
}

SLVAL
sl_array_length(sl_vm_t* vm, SLVAL array)
{
    return sl_make_int(vm, get_array(vm, array)->count);
}

SLVAL
sl_array_push(sl_vm_t* vm, SLVAL array, size_t count, SLVAL* items)
{
    sl_array_t* aryp = get_array(vm, array);
    size_t i, sz = aryp->count;
    array_resize(vm, aryp, aryp->count + count);
    for(i = 0; i < count; i++) {
        aryp->items[sz + i] = items[i];
    }
    return sl_make_int(vm, aryp->count);
}

SLVAL
sl_array_pop(sl_vm_t* vm, SLVAL array)
{
    sl_array_t* aryp = get_array(vm, array);
    SLVAL val = aryp->items[aryp->count - 1];
    array_resize(vm, aryp, aryp->count - 1);
    return val;
}

SLVAL
sl_array_unshift(sl_vm_t* vm, SLVAL array, size_t count, SLVAL* items)
{
    sl_array_t* aryp = get_array(vm, array);
    size_t i, sz = aryp->count;
    array_resize(vm, aryp, aryp->count + count);
    memmove(aryp->items + count, aryp->items, sz * sizeof(SLVAL));
    for(i = 0; i < count; i++) {
        aryp->items[i] = items[i];
    }
    return sl_make_int(vm, aryp->count);
}

SLVAL
sl_array_shift(sl_vm_t* vm, SLVAL array)
{
    sl_array_t* aryp = get_array(vm, array);
    SLVAL val = aryp->items[0];
    memmove(aryp->items, aryp->items + 1, (aryp->count - 1) * sizeof(SLVAL));
    array_resize(vm, aryp, aryp->count - 1);
    return val;
}
