#include <stdlib.h>
#include <string.h>
#include <gc.h>
#include "slash.h"

typedef struct {
    sl_object_t base;
    st_table_t* st;
}
sl_dict_t;

typedef struct {
    sl_vm_t* vm;
    SLVAL key;
}
sl_dict_key_t;

static int
dict_key_cmp(sl_dict_key_t* a, sl_dict_key_t* b)
{
    sl_vm_t* vm = a->vm;
    SLVAL cmp = sl_send(vm, a->key, "<=>", 1, b->key);
    if(sl_get_primitive_type(cmp) != SL_T_INT) {
        sl_throw_message2(vm, vm->lib.TypeError, "Expected <=> to return Int in Dict key");
    }
    return sl_get_int(cmp);
}

static int
dict_key_hash(sl_dict_key_t* a)
{
    return sl_hash(a->vm, a->key);
}

static struct st_hash_type
dict_hash_type = { dict_key_cmp, dict_key_hash };

static sl_object_t*
allocate_dict()
{
    sl_dict_t* dict = GC_MALLOC(sizeof(sl_dict_t));
    dict->st = st_init_table(&dict_hash_type);
    return (sl_object_t*)dict;
}

static sl_dict_t*
get_dict(sl_vm_t* vm, SLVAL dict)
{
    sl_expect(vm, dict, vm->lib.Dict);
    return (sl_dict_t*)sl_get_ptr(dict);
}

SLVAL
sl_make_dict(sl_vm_t* vm, size_t count, SLVAL* kvs)
{
    SLVAL dict = sl_new(vm, vm->lib.Dict, 0, NULL);
    size_t i;
    for(i = 0; i < count; i++) {
        sl_dict_set(vm, dict, kvs[i * 2], kvs[i * 2 + 1]);
    }
    return dict;
}

SLVAL
sl_dict_get(sl_vm_t* vm, SLVAL dict, SLVAL key)
{
    SLVAL val;
    sl_dict_key_t k;
    k.vm = vm;
    k.key = key;
    if(st_lookup(get_dict(vm, dict)->st, (st_data_t)&k, (st_data_t*)&val)) {
        return val;
    }
    return vm->lib.nil;
}

SLVAL
sl_dict_set(sl_vm_t* vm, SLVAL dict, SLVAL key, SLVAL val)
{
    sl_dict_key_t* k = GC_MALLOC(sizeof(sl_dict_key_t));
    k->vm = vm;
    k->key = key;
    st_insert(get_dict(vm, dict)->st, (st_data_t)k, (st_data_t)sl_get_ptr(val));
    return val;
}

SLVAL
sl_dict_length(sl_vm_t* vm, SLVAL dict)
{
    return sl_make_int(vm, get_dict(vm, dict)->st->num_entries);
}

SLVAL
sl_dict_delete(sl_vm_t* vm, SLVAL dict, SLVAL key)
{
    sl_dict_key_t k;
    sl_dict_key_t* pk = &k;
    k.vm = vm;
    k.key = key;
    if(st_delete(get_dict(vm, dict)->st, (st_data_t*)&pk, NULL)) {
        return vm->lib._true;
    } else {
        return vm->lib._false;
    }
}

void
sl_init_dict(sl_vm_t* vm)
{
    vm->lib.Dict = sl_define_class(vm, "Dict", vm->lib.Enumerable);
    sl_class_set_allocator(vm, vm->lib.Dict, allocate_dict);
    sl_define_method(vm, vm->lib.Dict, "[]", 1, sl_dict_get);
    sl_define_method(vm, vm->lib.Dict, "[]=", 2, sl_dict_set);
    sl_define_method(vm, vm->lib.Dict, "length", 0, sl_dict_length);
    sl_define_method(vm, vm->lib.Dict, "delete", 1, sl_dict_delete);
    /*
    sl_define_method(vm, vm->lib.Array, "enumerate", 0, sl_dict_enumerate);
    sl_define_method(vm, vm->lib.Array, "to_s", 0, sl_dict_to_s);
    sl_define_method(vm, vm->lib.Array, "inspect", 0, sl_dict_to_s);
    
    vm->lib.Dict_Enumerator = sl_define_class3(
        vm, sl_make_cstring(vm, "Enumerator"), vm->lib.Object, vm->lib.Dict);
        
    sl_class_set_allocator(vm, vm->lib.Dict_Enumerator, allocate_dict_enumerator);
    sl_define_method(vm, vm->lib.Dict_Enumerator, "init", 1, sl_dict_enumerator_init);
    sl_define_method(vm, vm->lib.Dict_Enumerator, "next", 0, sl_dict_enumerator_next);
    sl_define_method(vm, vm->lib.Dict_Enumerator, "current", 0, sl_dict_enumerator_current);
    */
}