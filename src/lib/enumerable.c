#include <slash.h>

static SLVAL
enumerable_map(sl_vm_t* vm, SLVAL self, SLVAL f)
{
    SLVAL array = sl_make_array(vm, 0, NULL);
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    SLVAL val;
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        val = sl_send_id(vm, enumerator, vm->id.current, 0);
        val = sl_send_id(vm, f, vm->id.call, 1, val);
        sl_array_push(vm, array, 1, &val);
    }
    return array;
}

static SLVAL
enumerable_to_a(sl_vm_t* vm, SLVAL self)
{
    SLVAL array = sl_make_array(vm, 0, NULL);
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    SLVAL val;
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        val = sl_send_id(vm, enumerator, vm->id.current, 0);
        sl_array_push(vm, array, 1, &val);
    }
    return array;
}

static SLVAL
enumerable_reduce(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    SLVAL acc, val, f;
    if(argc == 1) {
        if(!sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
            sl_throw_message2(vm, vm->lib.ArgumentError, "Can't reduce empty array without initializer");
        } else {
            acc = sl_send_id(vm, enumerator, vm->id.current, 0);
        }
        f = argv[0];
    } else {
        acc = argv[0];
        f = argv[1];
    }
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        val = sl_send_id(vm, enumerator, vm->id.current, 0);
        acc = sl_send_id(vm, f, vm->id.call, 2, acc, val);
    }
    return acc;
}

SLVAL
sl_enumerable_join(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    return sl_array_join(vm, enumerable_to_a(vm, self), argc, argv);
}

static SLVAL
enumerable_length(sl_vm_t* vm, SLVAL self)
{
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    int i = 0;
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        i++;
    }
    return sl_make_int(vm, i);
}

static SLVAL
enumerable_empty(sl_vm_t* vm, SLVAL self)
{
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    return sl_not(vm, sl_send_id(vm, enumerator, vm->id.next, 0));
}

static SLVAL
enumerable_any(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    SLVAL val;
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        val = sl_send_id(vm, enumerator, vm->id.current, 0);
        if(argc > 0) {
            val = sl_send_id(vm, argv[0], vm->id.call, 1, val);
        }
        if(sl_is_truthy(val)) {
            return vm->lib._true;
        }
    }
    return vm->lib._false;
}

static SLVAL
enumerable_all(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    SLVAL val;
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        val = sl_send_id(vm, enumerator, vm->id.current, 0);
        if(argc > 0) {
            val = sl_send_id(vm, argv[0], vm->id.call, 1, val);
        }
        if(!sl_is_truthy(val)) {
            return vm->lib._false;
        }
    }
    return vm->lib._true;
}

static SLVAL
enumerable_find(sl_vm_t* vm, SLVAL self, SLVAL f)
{
    SLVAL val, truthy;
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        val = sl_send_id(vm, enumerator, vm->id.current, 0);
        truthy = sl_send_id(vm, f, vm->id.call, 1, val);
        if(sl_is_truthy(truthy)) {
            return val;
        }
    }
    return vm->lib.nil;
}

static SLVAL
enumerable_filter(sl_vm_t* vm, SLVAL self, SLVAL f)
{
    SLVAL val, truthy, ary = sl_make_array(vm, 0, NULL);
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        val = sl_send_id(vm, enumerator, vm->id.current, 0);
        truthy = sl_send_id(vm, f, vm->id.call, 1, val);
        if(sl_is_truthy(truthy)) {
            sl_array_push(vm, ary, 1, &val);
        }
    }
    return ary;
}

static SLVAL
enumerable_reject(sl_vm_t* vm, SLVAL self, SLVAL f)
{
    SLVAL val, truthy, ary = sl_make_array(vm, 0, NULL);
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        val = sl_send_id(vm, enumerator, vm->id.current, 0);
        truthy = sl_send_id(vm, f, vm->id.call, 1, val);
        if(!sl_is_truthy(truthy)) {
            sl_array_push(vm, ary, 1, &val);
        }
    }
    return ary;
}

static SLVAL
enumerable_sort(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    return sl_array_sort(vm, enumerable_to_a(vm, self), argc, argv);
}

static SLVAL
enumerable_drop(sl_vm_t* vm, SLVAL self, SLVAL countv)
{
    SLVAL ary = sl_make_array(vm, 0, NULL);
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);

    for(int count = sl_get_int(countv); count > 0; count--) {
        if(!sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
            return ary;
        }
    }

    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        SLVAL elem = sl_send_id(vm, enumerator, vm->id.current, 0);
        sl_array_push(vm, ary, 1, &elem);
    }

    return ary;
}

static SLVAL
enumerable_take(sl_vm_t* vm, SLVAL self, SLVAL countv)
{
    SLVAL ary = sl_make_array(vm, 0, NULL);
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);

    for(int count = sl_get_int(countv); count > 0; count--) {
        if(!sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
            return ary;
        }
        SLVAL elem = sl_send_id(vm, enumerator, vm->id.current, 0);
        sl_array_push(vm, ary, 1, &elem);
    }

    return ary;
}

static SLVAL
enumerable_first(sl_vm_t* vm, SLVAL self)
{
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    if(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        return sl_send_id(vm, enumerator, vm->id.current, 0);
    } else {
        return vm->lib.nil;
    }
}

static SLVAL
enumerable_last(sl_vm_t* vm, SLVAL self)
{
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    SLVAL last = vm->lib.nil;
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        last = sl_send_id(vm, enumerator, vm->id.current, 0);
    }
    return last;
}

static SLVAL
enumerable_includes(sl_vm_t* vm, SLVAL self, SLVAL needle)
{
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        SLVAL current = sl_send_id(vm, enumerator, vm->id.current, 0);
        if(sl_eq(vm, needle, current)) {
            return vm->lib._true;
        }
    }
    return vm->lib._false;
}

static SLVAL
enumerable_each(sl_vm_t* vm, SLVAL self, SLVAL func)
{
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        SLVAL current = sl_send_id(vm, enumerator, vm->id.current, 0);
        sl_send_id(vm, func, vm->id.call, 1, current);
    }
    return self;
}

static SLVAL
enumerable_sum(sl_vm_t* vm, SLVAL self)
{
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    SLVAL acc;
    if(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        acc = sl_send_id(vm, enumerator, vm->id.current, 0);
    } else {
        return sl_make_int(vm, 0);
    }
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        SLVAL current = sl_send_id(vm, enumerator, vm->id.current, 0);
        acc = sl_send_id(vm, acc, vm->id.op_add, 1, current);
    }
    return acc;
}

static SLVAL
enumerable_average(sl_vm_t* vm, SLVAL self)
{
    SLVAL enumerator = sl_send_id(vm, self, vm->id.enumerate, 0);
    SLVAL acc;
    size_t count = 0;
    if(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        acc = sl_send_id(vm, enumerator, vm->id.current, 0);
        count++;
    } else {
        return vm->lib.nil;
    }
    while(sl_is_truthy(sl_send_id(vm, enumerator, vm->id.next, 0))) {
        SLVAL current = sl_send_id(vm, enumerator, vm->id.current, 0);
        acc = sl_send_id(vm, acc, vm->id.op_add, 1, current);
        count++;
    }
    SLVAL acc_f = sl_send_id(vm, acc, vm->id.to_f, 0);
    SLVAL count_f = sl_make_float(vm, (double)count);
    return sl_send_id(vm, acc_f, vm->id.op_div, 1, count_f);
}

void
sl_init_enumerable(sl_vm_t* vm)
{
    vm->lib.Enumerable = sl_define_class(vm, "Enumerable", vm->lib.Object);
    sl_define_method(vm, vm->lib.Enumerable, "map", 1, enumerable_map);
    sl_define_method(vm, vm->lib.Enumerable, "to_a", 0, enumerable_to_a);
    sl_define_method(vm, vm->lib.Enumerable, "reduce", -2, enumerable_reduce);
    sl_define_method(vm, vm->lib.Enumerable, "fold", -2, enumerable_reduce);
    sl_define_method(vm, vm->lib.Enumerable, "join", -1, sl_enumerable_join);
    sl_define_method(vm, vm->lib.Enumerable, "length", 0, enumerable_length);
    sl_define_method(vm, vm->lib.Enumerable, "empty", 0, enumerable_empty);
    sl_define_method(vm, vm->lib.Enumerable, "any", -1, enumerable_any);
    sl_define_method(vm, vm->lib.Enumerable, "all", -1, enumerable_all);
    sl_define_method(vm, vm->lib.Enumerable, "find", 1, enumerable_find);
    sl_define_method(vm, vm->lib.Enumerable, "filter", 1, enumerable_filter);
    sl_define_method(vm, vm->lib.Enumerable, "reject", 1, enumerable_reject);
    sl_define_method(vm, vm->lib.Enumerable, "sort", -1, enumerable_sort);
    sl_define_method(vm, vm->lib.Enumerable, "drop", 1, enumerable_drop);
    sl_define_method(vm, vm->lib.Enumerable, "take", 1, enumerable_take);
    sl_define_method(vm, vm->lib.Enumerable, "first", 0, enumerable_first);
    sl_define_method(vm, vm->lib.Enumerable, "last", 0, enumerable_last);
    sl_define_method(vm, vm->lib.Enumerable, "includes", 1, enumerable_includes);
    sl_define_method(vm, vm->lib.Enumerable, "each", 1, enumerable_each);
    sl_define_method(vm, vm->lib.Enumerable, "sum", 0, enumerable_sum);
    sl_define_method(vm, vm->lib.Enumerable, "average", 0, enumerable_average);
}
