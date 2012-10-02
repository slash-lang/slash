#include <stdlib.h>
#include <string.h>
#include <slash.h>

typedef struct {
    sl_object_t base;
    int inspecting;
    st_table_t* st;
}
sl_dict_t;

typedef struct {
    sl_vm_t* vm;
    SLVAL key;
}
sl_dict_key_t;

typedef struct {
    sl_object_t base;
    SLVAL dict;
    SLVAL* keys;
    size_t count;
    size_t at;
}
sl_dict_enumerator_t;

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
allocate_dict(sl_vm_t* vm)
{
    sl_dict_t* dict = sl_alloc(vm->arena, sizeof(sl_dict_t));
    dict->base.primitive_type = SL_T_DICT;
    dict->st = st_init_table(vm->arena, &dict_hash_type);
    return (sl_object_t*)dict;
}

static sl_dict_t*
get_dict(sl_vm_t* vm, SLVAL dict)
{
    sl_expect(vm, dict, vm->lib.Dict);
    return (sl_dict_t*)sl_get_ptr(dict);
}

static sl_dict_enumerator_t*
get_dict_enumerator(sl_vm_t* vm, SLVAL e)
{
    sl_expect(vm, e, vm->lib.Dict_Enumerator);
    return (sl_dict_enumerator_t*)sl_get_ptr(e);
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
    sl_dict_key_t* k = sl_alloc(vm->arena, sizeof(sl_dict_key_t));
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

static int
sl_dict_merge_iter(sl_dict_key_t* key, SLVAL value, SLVAL dict)
{
    sl_dict_set(key->vm, dict, key->key, value);
    return ST_CONTINUE;
}

SLVAL
sl_dict_merge(sl_vm_t* vm, SLVAL dict, SLVAL other)
{
    sl_dict_t* a = get_dict(vm, dict);
    sl_dict_t* b = get_dict(vm, other);
    SLVAL new_dict = sl_new(vm, vm->lib.Dict, 0, NULL);
    st_foreach(a->st, sl_dict_merge_iter, (st_data_t)sl_get_ptr(new_dict));
    st_foreach(b->st, sl_dict_merge_iter, (st_data_t)sl_get_ptr(new_dict));
    return new_dict;
}

static SLVAL
sl_dict_enumerate(sl_vm_t* vm, SLVAL dict)
{
    return sl_new(vm, vm->lib.Dict_Enumerator, 1, &dict);
}

static int
dict_to_s_iter(sl_dict_key_t* key, SLVAL value, SLVAL* str)
{
    if(sl_get_int(sl_string_length(key->vm, *str)) > 2) {
        *str = sl_string_concat(key->vm, *str, sl_make_cstring(key->vm, ", "));
    }
    *str = sl_string_concat(key->vm, *str, sl_inspect(key->vm, key->key));
    *str = sl_string_concat(key->vm, *str, sl_make_cstring(key->vm, " => "));
    *str = sl_string_concat(key->vm, *str, sl_inspect(key->vm, value));
    return ST_CHECK;
}

static SLVAL
sl_dict_to_s(sl_vm_t* vm, SLVAL dict)
{
    sl_catch_frame_t frame;
    SLVAL str;
    sl_dict_t* d = get_dict(vm, dict);
    if(d->inspecting) {
        return sl_make_cstring(vm, "{ <recursive> }");
    }
    SL_ENSURE(frame, {
        d->inspecting = 1;
        str = sl_make_cstring(vm, "{ ");
        st_foreach(d->st, dict_to_s_iter, (st_data_t)&str);
        str = sl_string_concat(vm, str, sl_make_cstring(vm, " }"));
    }, {
        d->inspecting = 0;
    });
    return str;
}

static sl_object_t*
allocate_dict_enumerator(sl_vm_t* vm)
{
    return sl_alloc(vm->arena, sizeof(sl_dict_enumerator_t));
}

static int
dict_enumerator_init_iter(sl_dict_key_t* key, SLVAL record, sl_dict_enumerator_t* e)
{
    (void)record;
    e->keys[e->at] = key->key;
    e->at++;
    return ST_CONTINUE;
}

static SLVAL
sl_dict_enumerator_init(sl_vm_t* vm, SLVAL self, SLVAL dict)
{
    sl_dict_t* d = get_dict(vm, dict);
    sl_dict_enumerator_t* e = get_dict_enumerator(vm, self);
    e->dict = dict;
    e->count = d->st->num_entries;
    e->keys = sl_alloc(vm->arena, sizeof(SLVAL) * e->count);
    st_foreach(d->st, dict_enumerator_init_iter, (st_data_t)e);
    e->at = 0;
    return self;
}

static SLVAL
sl_dict_enumerator_next(sl_vm_t* vm, SLVAL self)
{
    sl_dict_enumerator_t* e = get_dict_enumerator(vm, self);
    if(!e->keys) {
        sl_throw_message2(vm, vm->lib.Error, "Invalid operation on Dict::Enumerator");
    }
    if(e->at > e->count) {
        return vm->lib._false;
    } else if(e->at == e->count) {
        e->at++;
        return vm->lib._false;
    } else {
        e->at++;
        return vm->lib._true;
    }
}

static SLVAL
sl_dict_enumerator_current(sl_vm_t* vm, SLVAL self)
{
    sl_dict_enumerator_t* e = get_dict_enumerator(vm, self);
    SLVAL kv[2];
    if(!e->keys) {
        sl_throw_message2(vm, vm->lib.Error, "Invalid operation on Dict::Enumerator");
    }
    if(e->at == 0 || e->at > e->count) {
        sl_throw_message2(vm, vm->lib.Error, "Invalid operation on Dict::Enumerator");
    }
    kv[0] = e->keys[e->at - 1];
    kv[1] = sl_dict_get(vm, e->dict, kv[0]);
    return sl_make_array(vm, 2, kv);
}

struct dict_keys_state {
    SLVAL* keys;
    size_t at;
};

static int
sl_dict_keys_iter(sl_dict_key_t* key, SLVAL value, struct dict_keys_state* state)
{
    (void)value;
    state->keys[state->at++] = key->key;
    return ST_CONTINUE;
}

SLVAL*
sl_dict_keys(sl_vm_t* vm, SLVAL dict, size_t* count)
{
    sl_dict_t* d = get_dict(vm, dict);
    struct dict_keys_state state;
    state.keys = sl_alloc(vm->arena, sizeof(SLVAL) * d->st->num_entries);
    state.at = 0;
    st_foreach(d->st, sl_dict_keys_iter, (st_data_t)&state);
    *count = state.at;
    return state.keys;
}

struct dict_eq_iter_state {
    sl_vm_t* vm;
    sl_dict_t* other;
    int success;
};

static int
dict_eq_iter(sl_dict_key_t* key, SLVAL value, struct dict_eq_iter_state* state)
{
    SLVAL other_value;
    if(!st_lookup(state->other->st, (st_data_t)key, (st_data_t*)&other_value)) {
        state->success = 0;
        return ST_STOP;
    }
    if(!sl_eq(state->vm, value, other_value)) {
        state->success = 0;
        return ST_STOP;
    }
    return ST_CONTINUE;
}

static SLVAL
sl_dict_eq(sl_vm_t* vm, SLVAL dict, SLVAL other)
{
    if(!sl_is_a(vm, other, vm->lib.Dict)) {
        return vm->lib._false;
    }
    sl_dict_t* d = get_dict(vm, dict);
    sl_dict_t* o = get_dict(vm, other);
    struct dict_eq_iter_state state;
    state.vm = vm;
    state.other = o;
    state.success = 1;
    if(d->st->num_entries != o->st->num_entries) {
        return vm->lib._false;
    }
    st_foreach(d->st, dict_eq_iter, (st_data_t)&state);
    return state.success ? vm->lib._true : vm->lib._false;
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
    sl_define_method(vm, vm->lib.Dict, "merge", 1, sl_dict_merge);
    sl_define_method(vm, vm->lib.Dict, "enumerate", 0, sl_dict_enumerate);
    sl_define_method(vm, vm->lib.Dict, "to_s", 0, sl_dict_to_s);
    sl_define_method(vm, vm->lib.Dict, "inspect", 0, sl_dict_to_s);
    sl_define_method(vm, vm->lib.Dict, "==", 1, sl_dict_eq);
    
    vm->lib.Dict_Enumerator = sl_define_class3(
        vm, sl_make_cstring(vm, "Enumerator"), vm->lib.Object, vm->lib.Dict);
        
    sl_class_set_allocator(vm, vm->lib.Dict_Enumerator, allocate_dict_enumerator);
    sl_define_method(vm, vm->lib.Dict_Enumerator, "init", 1, sl_dict_enumerator_init);
    sl_define_method(vm, vm->lib.Dict_Enumerator, "next", 0, sl_dict_enumerator_next);
    sl_define_method(vm, vm->lib.Dict_Enumerator, "current", 0, sl_dict_enumerator_current);
}
