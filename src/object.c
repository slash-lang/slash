#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <slash/object.h>
#include <slash/class.h>
#include <slash/string.h>
#include <slash/method.h>
#include <slash/platform.h>
#include <slash/lib/array.h>

void
sl_pre_init_object(sl_vm_t* vm)
{
    sl_class_t* klass;
    klass = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    klass->super = vm->lib.nil;
    klass->name.id = 0;
    klass->in = vm->lib.nil;
    klass->constants = st_init_table(vm->arena, &sl_id_hash_type);
    klass->class_variables = st_init_table(vm->arena, &sl_id_hash_type);
    klass->instance_methods = st_init_table(vm->arena, &sl_id_hash_type);
    klass->base.klass = vm->lib.Class;
    klass->base.primitive_type = SL_T_CLASS;
    klass->base.instance_variables = st_init_table(vm->arena, &sl_id_hash_type);
}

static SLVAL
sl_object_send(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv);

static SLVAL
sl_object_eq(sl_vm_t* vm, SLVAL self, SLVAL other);

static SLVAL
sl_object_ne(sl_vm_t* vm, SLVAL self, SLVAL other);

static SLVAL
sl_object_hash(sl_vm_t* vm, SLVAL self);

static SLVAL
sl_object_is_a(sl_vm_t* vm, SLVAL self, SLVAL klass);

static SLVAL
sl_object_method(sl_vm_t* vm, SLVAL self, SLVAL method_name);

static SLVAL
sl_object_own_method(sl_vm_t* vm, SLVAL self, SLVAL method_name);

static SLVAL
sl_object_own_methods(sl_vm_t* vm, SLVAL self);

static SLVAL
sl_object_methods(sl_vm_t* vm, SLVAL self);

static SLVAL
sl_responds_to_slval(sl_vm_t* vm, SLVAL object, SLVAL idv);

void
sl_init_object(sl_vm_t* vm)
{
    sl_class_t* objectp = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    st_insert(objectp->constants, (st_data_t)sl_intern(vm, "Object").id, (st_data_t)sl_get_ptr(vm->lib.Object));
    sl_define_method(vm, vm->lib.Object, "to_s", 0, sl_object_to_s);
    sl_define_method(vm, vm->lib.Object, "inspect", 0, sl_object_inspect);
    sl_define_method(vm, vm->lib.Object, "send", -2, sl_object_send);
    sl_define_method(vm, vm->lib.Object, "responds_to", 1, sl_responds_to_slval);
    sl_define_method(vm, vm->lib.Object, "class", 0, sl_class_of);
    sl_define_method(vm, vm->lib.Object, "singleton_class", 0, sl_singleton_class);
    sl_define_method(vm, vm->lib.Object, "is_a", 1, sl_object_is_a);
    sl_define_method(vm, vm->lib.Object, "is_an", 1, sl_object_is_a);
    sl_define_method(vm, vm->lib.Object, "hash", 0, sl_object_hash);
    sl_define_method(vm, vm->lib.Object, "method", 1, sl_object_method);
    sl_define_method(vm, vm->lib.Object, "own_method", 1, sl_object_own_method);
    sl_define_method(vm, vm->lib.Object, "own_methods", 0, sl_object_own_methods);
    sl_define_method(vm, vm->lib.Object, "methods", 0, sl_object_methods);
    sl_define_method(vm, vm->lib.Object, "==", 1, sl_object_eq);
    sl_define_method(vm, vm->lib.Object, "!=", 1, sl_object_ne);
}

static SLVAL
sl_object_send(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    SLID mid = sl_intern2(vm, argv[0]);
    return sl_send2(vm, self, mid, argc - 1, argv + 1);
}

int
sl_eq(sl_vm_t* vm, SLVAL a, SLVAL b)
{
    return sl_is_truthy(sl_send_id(vm, a, vm->id.op_eq, 1, b));
}

static SLVAL
sl_object_eq(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    return sl_make_bool(vm, sl_get_ptr(self) == sl_get_ptr(other));
}

static SLVAL
sl_object_ne(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    SLVAL value = sl_send_id(vm, self, vm->id.op_eq, 1, other);
    return sl_make_bool(vm, !sl_is_truthy(value));
}

static SLVAL
sl_object_hash(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, (int)(intptr_t)sl_get_ptr(self));
}

static SLVAL
sl_object_is_a(sl_vm_t* vm, SLVAL self, SLVAL klass)
{
    return sl_make_bool(vm, sl_is_a(vm, self, klass));
}

SLVAL
sl_to_s(sl_vm_t* vm, SLVAL obj)
{
    SLVAL s = sl_send_id(vm, obj, vm->id.to_s, 0);
    if(sl_get_primitive_type(s) == SL_T_STRING) {
        return s;
    } else {
        return sl_object_inspect(vm, obj);
    }
}

SLVAL
sl_to_s_no_throw(sl_vm_t* vm, SLVAL obj)
{
    sl_vm_frame_t frame;
    SLVAL ret, err;
    SL_TRY(frame, SL_UNWIND_EXCEPTION, {
        ret = sl_to_s(vm, obj);
    }, err, {
        (void)err;
        ret = sl_object_inspect(vm, obj);
    });
    return ret;
}

SLVAL
sl_inspect(sl_vm_t* vm, SLVAL obj)
{
    SLVAL s = sl_send_id(vm, obj, vm->id.inspect, 0);
    if(sl_get_primitive_type(s) == SL_T_STRING) {
        return s;
    } else {
        return sl_object_inspect(vm, obj);
    }
}

char*
sl_to_cstr(sl_vm_t* vm, SLVAL obj)
{
    SLVAL str = sl_to_s(vm, obj);
    sl_string_t* strp = (sl_string_t*)sl_get_ptr(str);
    char* buff = sl_alloc_buffer(vm->arena,strp->buff_len + 1);
    memcpy(buff, strp->buff, strp->buff_len);
    buff[strp->buff_len] = 0;
    return buff;
}

SLVAL
sl_object_to_s(sl_vm_t* vm, SLVAL self)
{
    return sl_send_id(vm, self, vm->id.inspect, 0);
}

SLVAL
sl_object_inspect(sl_vm_t* vm, SLVAL self)
{
    char buff[128];
    SLVAL klass = sl_class_of(vm, self);
    SLVAL str = sl_make_cstring(vm, "#<");
    str = sl_string_concat(vm, str, sl_to_s(vm, klass));
    sprintf(buff, ":%p>", (void*)sl_get_ptr(self));
    str = sl_string_concat(vm, str, sl_make_cstring(vm, buff));
    return str;
}

void
sl_define_singleton_method(sl_vm_t* vm, SLVAL object, char* name, int arity, SLVAL(*func)())
{
    sl_define_singleton_method2(vm, object, sl_intern(vm, name), arity, func);
}

void
sl_define_singleton_method2(sl_vm_t* vm, SLVAL object, SLID name, int arity, SLVAL(*func)())
{
    SLVAL method = sl_make_c_func(vm, sl_class_of(vm, object), name, arity, func);
    sl_define_singleton_method3(vm, object, name, method);
}

void
sl_define_singleton_method3(sl_vm_t* vm, SLVAL object, SLID name, SLVAL method)
{
    sl_expect(vm, method, vm->lib.Method);
    if(sl_is_a(vm, object, vm->lib.Int)) {
        sl_throw_message2(vm, vm->lib.TypeError, "Can't define singleton method on Int object");
    }
    SLVAL singleton_class = sl_singleton_class(vm, object);
    sl_define_method3(vm, singleton_class, name, method);
}

int
sl_responds_to(sl_vm_t* vm, SLVAL object, char* id)
{
    return sl_responds_to2(vm, object, sl_intern(vm, id));
}

static SLVAL
sl_responds_to_slval(sl_vm_t* vm, SLVAL object, SLVAL idv)
{
    return sl_make_bool(vm, sl_responds_to2(vm, object, sl_intern2(vm, idv)));
}

int
sl_responds_to2(sl_vm_t* vm, SLVAL object, SLID id)
{
    return sl_lookup_method(vm, object, id) != NULL;
}

SLVAL
sl_get_ivar(sl_vm_t* vm, SLVAL object, SLID id)
{
    sl_object_t* p;
    SLVAL val;
    if(sl_is_a(vm, object, vm->lib.Int)) {
        return vm->lib.nil;
    }
    p = sl_get_ptr(object);
    if(p->instance_variables && st_lookup(p->instance_variables, (st_data_t)id.id, (st_data_t*)&val)) {
        return val;
    }
    return vm->lib.nil;
}

SLVAL
sl_get_cvar(sl_vm_t* vm, SLVAL object, SLID id)
{
    SLVAL val;
    if(!sl_is_a(vm, object, vm->lib.Class)) {
        object = sl_class_of(vm, object);
    }
    while(sl_get_primitive_type(object) == SL_T_CLASS) {
        sl_class_t* p = (sl_class_t*)sl_get_ptr(object);
        if(st_lookup(p->class_variables, (st_data_t)id.id, (st_data_t*)&val)) {
            return val;
        }
        object = p->super;
    }
    return vm->lib.nil;
}

void
sl_set_ivar(sl_vm_t* vm, SLVAL object, SLID id, SLVAL val)
{
    sl_object_t* p;
    if(sl_is_a(vm, object, vm->lib.Int)) {
        sl_throw_message2(vm, vm->lib.TypeError, "Can't set instance variable on Int");
    }
    p = sl_get_ptr(object);
    if(!p->instance_variables) {
        p->instance_variables = st_init_table(vm->arena, &sl_id_hash_type);
    }
    st_insert(p->instance_variables, (st_data_t)id.id, (st_data_t)sl_get_ptr(val));
}

void
sl_set_cvar(sl_vm_t* vm, SLVAL object, SLID id, SLVAL val)
{
    sl_class_t* p;
    if(!sl_is_a(vm, object, vm->lib.Class)) {
        object = sl_class_of(vm, object);
    }
    p = (sl_class_t*)sl_get_ptr(object);
    st_insert(p->class_variables, (st_data_t)id.id, (st_data_t)sl_get_ptr(val));
}

SLVAL
sl_send_id(sl_vm_t* vm, SLVAL recv, SLID id, size_t argc, ...)
{
    SLVAL argv[argc];
    va_list va;
    size_t i;
    va_start(va, argc);
    for(i = 0; i < argc; i++) {
        argv[i] = va_arg(va, SLVAL);
    }
    va_end(va);
    return sl_send2(vm, recv, id, argc, argv);
}

SLVAL
sl_send(sl_vm_t* vm, SLVAL recv, char* id, size_t argc, ...)
{
    SLVAL* argv = alloca(argc * sizeof(SLVAL));
    va_list va;
    size_t i;
    va_start(va, argc);
    for(i = 0; i < argc; i++) {
        argv[i] = va_arg(va, SLVAL);
    }
    va_end(va);
    return sl_send2(vm, recv, sl_intern(vm, id), argc, argv);
}

static SLVAL
call_c_func(sl_vm_t* vm, SLVAL recv, sl_method_t* method, size_t argc, SLVAL* argv)
{
    if(method->arity < 0) {
        return method->as.c.func(vm, recv, argc, argv);
    } else {
        switch(method->arity) {
            case 0: return method->as.c.func(vm, recv);
            case 1: return method->as.c.func(vm, recv, argv[0]);
            case 2: return method->as.c.func(vm, recv, argv[0], argv[1]);
            case 3: return method->as.c.func(vm, recv, argv[0], argv[1], argv[2]);
            case 4: return method->as.c.func(vm, recv, argv[0], argv[1], argv[2], argv[3]);
            default:
                sl_throw_message(vm, "Too many arguments for C function");
        }
    }
    return vm->lib.nil; /* never reached */
}

static SLVAL
call_c_func_guard(sl_vm_t* vm, SLVAL recv, sl_method_t* method, size_t argc, SLVAL* argv)
{
    sl_vm_frame_t frame;
    frame.prev = vm->call_stack;
    frame.frame_type = SL_VM_FRAME_C;
    frame.as.call_frame.method = method->name;
    vm->call_stack = &frame;
    
    SLVAL retn = call_c_func(vm, recv, method, argc, argv);
    
    vm->call_stack = frame.prev;
    return retn;
}

SLVAL
sl_apply_method(sl_vm_t* vm, SLVAL recv, sl_method_t* method, size_t argc, SLVAL* argv)
{
    char errstr[1024];
    sl_vm_exec_ctx_t stack_ctx;
    sl_vm_exec_ctx_t* ctx;
    size_t i;
    SLVAL arg;
    if((void*)&arg < vm->stack_limit) {
        /* we're about to blow the stack */
        sl_throw(vm, vm->lib.StackOverflowError_instance);
    }
    if(method->arity < 0) {
        if(sl_unlikely((size_t)(-method->arity - 1) > argc)) {
            sprintf(errstr, "Too few arguments. Expected %d, received %lu.", (-method->arity - 1), (unsigned long)argc);
            sl_throw_message2(vm, vm->lib.ArgumentError, errstr);
        }
    } else {
        if(sl_unlikely((size_t)method->arity > argc)) {
            sprintf(errstr, "Too few arguments. Expected %d, received %lu.", method->arity, (unsigned long)argc);
            sl_throw_message2(vm, vm->lib.ArgumentError, errstr);
        }
        if(sl_unlikely((size_t)method->arity < argc)) {
            sprintf(errstr, "Too many arguments. Expected %d, received %lu.", method->arity, (unsigned long)argc);
            sl_throw_message2(vm, vm->lib.ArgumentError, errstr);
        }
    }
    if(method->is_c_func) {
        return call_c_func_guard(vm, recv, method, argc, argv);
    } else {
        if(sl_likely(method->as.sl.section->can_stack_alloc_frame)) {
            ctx = &stack_ctx;
            memset(ctx, 0, sizeof(*ctx));
        } else {
            ctx = sl_alloc(vm->arena, sizeof(sl_vm_exec_ctx_t));
        }
        ctx->vm = vm;
        ctx->section = method->as.sl.section;
        if(method->as.sl.section->can_stack_alloc_frame) {
            ctx->registers = alloca(sizeof(SLVAL) * ctx->section->max_registers);
        } else {
            ctx->registers = sl_alloc(vm->arena, sizeof(SLVAL) * ctx->section->max_registers);
        }
        ctx->self = recv;
        ctx->parent = method->as.sl.parent_ctx;
        
        for(i = 0; i < ctx->section->arg_registers; i++) {
            ctx->registers[i + 1] = argv[i];
        }
        
        if(argc > ctx->section->arg_registers) {
            argc = ctx->section->arg_registers;
        }
        if(ctx->section->opt_skip) {
            return sl_vm_exec(ctx, ctx->section->opt_skip[argc - ctx->section->req_registers]);
        } else {
            return sl_vm_exec(ctx, 0);
        }
    }
    return vm->lib.nil;
}

static sl_method_t*
lookup_method_rec(sl_vm_t* vm, SLVAL klass, SLID id)
{
    sl_class_t* klassp = (sl_class_t*)sl_get_ptr(klass);
    if(klassp->base.primitive_type == SL_T_NIL) {
        return NULL;
    }

    sl_method_t* method;
    if(st_lookup(klassp->instance_methods, (st_data_t)id.id, (st_data_t*)&method)) {
        if(method->base.primitive_type == SL_T_CACHED_METHOD_ENTRY) {
            sl_cached_method_entry_t* cme = (void*)method;
            // TODO - improve cache invalidation. this is too coarse
            if(cme->state == vm->state_method) {
                return cme->method;
            }
        } else {
            return method;
        }
    }

    method = lookup_method_rec(vm, klassp->super, id);
    sl_cached_method_entry_t* cme = sl_alloc(vm->arena, sizeof(*cme));
    cme->primitive_type = SL_T_CACHED_METHOD_ENTRY;
    cme->state = vm->state_method;
    cme->method = method;
    st_insert(klassp->instance_methods, (st_data_t)id.id, (st_data_t)cme);
    return method;
}

sl_method_t*
sl_lookup_method(sl_vm_t* vm, SLVAL recv, SLID id)
{
    SLVAL klass = SL_IS_INT(recv) ? vm->lib.Int : sl_get_ptr(recv)->klass;
    return lookup_method_rec(vm, klass, id);
}

SLVAL
sl_send2(sl_vm_t* vm, SLVAL recv, SLID id, size_t argc, SLVAL* argv)
{
    sl_method_t* method = sl_lookup_method(vm, recv, id);
    if(sl_likely(method != NULL)) {
        return sl_apply_method(vm, recv, method, argc, argv);
    }
    
    /* look for method_missing method */
    
    SLVAL* argv2 = sl_alloc(vm->arena, (argc + 1) * sizeof(SLVAL));
    memcpy(argv2 + 1, argv, sizeof(SLVAL) * argc);
    argv2[0] = sl_id_to_string(vm, id);
    
    method = sl_lookup_method(vm, recv, vm->id.method_missing);
    if(method) {
        return sl_apply_method(vm, recv, method, argc + 1, argv2);
    }
    
    /* nope */
    
    SLVAL error = sl_make_cstring(vm, "Undefined method '");
    error = sl_string_concat(vm, error, sl_id_to_string(vm, id));
    error = sl_string_concat(vm, error, sl_make_cstring(vm, "' on "));
    error = sl_string_concat(vm, error, sl_object_inspect(vm, recv));
    sl_throw(vm, sl_make_error2(vm, vm->lib.NoMethodError, error));
    return vm->lib.nil; /* shutup gcc */
}

static SLVAL
sl_object_method(sl_vm_t* vm, SLVAL self, SLVAL method_name)
{
    SLID mid = sl_intern2(vm, method_name);
    sl_method_t* method = sl_lookup_method(vm, self, mid);
    if(method) {
        return sl_method_bind(vm, sl_make_ptr((sl_object_t*)method), self);
    } else {
        return vm->lib.nil;
    }
}

static SLVAL
sl_object_own_method(sl_vm_t* vm, SLVAL self, SLVAL method_name)
{
    SLID mid = sl_intern2(vm, method_name);
    SLVAL klass = SL_IS_INT(self) ? vm->lib.Int : sl_get_ptr(self)->klass;
    SLVAL method;
    sl_class_t* klassp;

    do {
        klassp = (sl_class_t*)sl_get_ptr(klass);
        if(st_lookup(klassp->instance_methods, (st_data_t)mid.id, (st_data_t*)&method)) {
            if(sl_get_ptr(method)->primitive_type != SL_T_CACHED_METHOD_ENTRY) {
                return method;
            }
        }
        klass = klassp->super;
    } while(klassp->singleton);

    return vm->lib.nil;
}

struct collect_methods_iter_state {
    sl_vm_t* vm;
    SLVAL ary;
};

static int
collect_methods_iter(SLID id, SLVAL method, struct collect_methods_iter_state* state)
{
    if(sl_get_primitive_type(method) != SL_T_CACHED_METHOD_ENTRY) {
        SLVAL name = sl_id_to_string(state->vm, id);
        sl_array_push(state->vm, state->ary, 1, &name);
    }
    return ST_CONTINUE;
}

static SLVAL
sl_object_own_methods(sl_vm_t* vm, SLVAL self)
{
    SLVAL klass = SL_IS_INT(self) ? vm->lib.Int : sl_get_ptr(self)->klass;
    sl_class_t* klassp;

    struct collect_methods_iter_state state;
    state.vm = vm;
    state.ary = sl_make_array(vm, 0, NULL);

    do {
        klassp = (sl_class_t*)sl_get_ptr(klass);
        st_foreach(klassp->instance_methods, collect_methods_iter, (st_data_t)&state);
        klass = klassp->super;
    } while(klassp->singleton);

    return state.ary;
}

static SLVAL
sl_object_methods(sl_vm_t* vm, SLVAL self)
{
    SLVAL klass = SL_IS_INT(self) ? vm->lib.Int : sl_get_ptr(self)->klass;

    struct collect_methods_iter_state state;
    state.vm = vm;
    state.ary = sl_make_array(vm, 0, NULL);

    while(sl_get_primitive_type(klass) == SL_T_CLASS) {
        sl_class_t* klassp = (sl_class_t*)sl_get_ptr(klass);
        st_foreach(klassp->instance_methods, collect_methods_iter, (st_data_t)&state);
        klass = klassp->super;
    }

    return state.ary;
}

SLVAL
sl_singleton_class(sl_vm_t* vm, SLVAL object)
{
    if(sl_get_primitive_type(object) == SL_T_INT) {
        sl_throw_message2(vm, vm->lib.TypeError, "can't get singleton class for Int");
    }
    sl_object_t* ptr = sl_get_ptr(object);
    sl_class_t* klass = (sl_class_t*)sl_get_ptr(ptr->klass);
    if(klass->singleton) {
        return ptr->klass;
    }
    SLVAL singleton_class = sl_make_class(vm, ptr->klass);
    ((sl_class_t*)sl_get_ptr(singleton_class))->singleton = true;
    return ptr->klass = singleton_class;
}
