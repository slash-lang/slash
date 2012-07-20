#include <alloca.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gc.h>
#include "object.h"
#include "class.h"
#include "string.h"
#include "method.h"

void
sl_pre_init_object(sl_vm_t* vm)
{
    sl_class_t* klass;
    klass = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    klass->super = vm->lib.nil;
    klass->name = vm->lib.nil;
    klass->in = vm->lib.nil;
    klass->constants = st_init_table(&sl_string_hash_type);
    klass->class_variables = st_init_table(&sl_string_hash_type);
    klass->instance_methods = st_init_table(&sl_string_hash_type);
    klass->base.klass = vm->lib.Class;
    klass->base.primitive_type = SL_T_CLASS;
    klass->base.instance_variables = st_init_table(&sl_string_hash_type);
}

static SLVAL
sl_object_send(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv);

static SLVAL
sl_object_eq(sl_vm_t* vm, SLVAL self, SLVAL other);

static SLVAL
sl_object_ne(sl_vm_t* vm, SLVAL self, SLVAL other);

void
sl_init_object(sl_vm_t* vm)
{
    st_insert(((sl_class_t*)sl_get_ptr(vm->lib.Object))->constants,
        (st_data_t)sl_cstring(vm, "Object"), (st_data_t)vm->lib.Object.i);
    sl_define_method(vm, vm->lib.Object, "to_s", 0, sl_object_to_s);
    sl_define_method(vm, vm->lib.Object, "inspect", 0, sl_object_to_s);
    sl_define_method(vm, vm->lib.Object, "send", -2, sl_object_send);
    sl_define_method(vm, vm->lib.Object, "class", 0, sl_class_of);
    sl_define_method(vm, vm->lib.Object, "==", 1, sl_object_eq);
    sl_define_method(vm, vm->lib.Object, "!=", 1, sl_object_ne);
}

static SLVAL
sl_object_send(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    return sl_send2(vm, self, argv[0], argc - 1, argv + 1);
}

static SLVAL
sl_object_eq(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(sl_get_ptr(self) == sl_get_ptr(other)) {
        return vm->lib._true;
    } else {
        return vm->lib._false;
    }
}

static SLVAL
sl_object_ne(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(sl_is_truthy(sl_send(vm, self, "==", 1, other))) {
        return vm->lib._false;
    } else {
        return vm->lib._true;
    }
}

SLVAL
sl_to_s(sl_vm_t* vm, SLVAL obj)
{
    SLVAL s = sl_send(vm, obj, "to_s", 0);
    if(sl_get_primitive_type(s) == SL_T_STRING) {
        return s;
    } else {
        return sl_object_to_s(vm, obj);
    }
}

SLVAL
sl_inspect(sl_vm_t* vm, SLVAL obj)
{
    SLVAL s = sl_send(vm, obj, "inspect", 0);
    if(sl_get_primitive_type(s) == SL_T_STRING) {
        return s;
    } else {
        return sl_object_to_s(vm, obj);
    }
}

char*
sl_to_cstr(sl_vm_t* vm, SLVAL obj)
{
    SLVAL str = sl_to_s(vm, obj);
    sl_string_t* strp = (sl_string_t*)sl_get_ptr(str);
    char* buff = (char*)GC_MALLOC_ATOMIC(strp->buff_len + 1);
    memcpy(buff, strp->buff, strp->buff_len);
    buff[strp->buff_len] = 0;
    return buff;
}

SLVAL
sl_object_to_s(sl_vm_t* vm, SLVAL self)
{
    SLVAL klass = sl_class_of(vm, self);
    sl_class_t* klassp = (sl_class_t*)sl_get_ptr(klass);
    sl_string_t* name = (sl_string_t*)sl_get_ptr(klassp->name);
    char* str = (char*)GC_MALLOC_ATOMIC(32 + name->buff_len);
    strcpy(str, "#<");
    memcpy(str + 2, name->buff, name->buff_len);
    sprintf(str + 2 + name->buff_len, ":%p>", (void*)sl_get_ptr(self));
    return sl_make_cstring(vm, str);
}

void
sl_define_singleton_method(sl_vm_t* vm, SLVAL object, char* name, int arity, SLVAL(*func)())
{
    sl_define_singleton_method2(vm, object, sl_make_cstring(vm, name), arity, func);
}

void
sl_define_singleton_method2(sl_vm_t* vm, SLVAL object, SLVAL name, int arity, SLVAL(*func)())
{
    SLVAL method = sl_make_c_func(vm, name, arity, func);
    sl_define_singleton_method3(vm, object, name, method);
}

void
sl_define_singleton_method3(sl_vm_t* vm, SLVAL object, SLVAL name, SLVAL method)
{
    sl_expect(vm, name, vm->lib.String);
    sl_expect(vm, method, vm->lib.Method);
    if(!sl_get_ptr(object)->singleton_methods) {
        sl_get_ptr(object)->singleton_methods = st_init_table(&sl_string_hash_type);
    }
    st_insert(sl_get_ptr(object)->singleton_methods, (st_data_t)sl_get_ptr(name), (st_data_t)sl_get_ptr(method));
}

SLVAL
sl_responds_to(sl_vm_t* vm, SLVAL object, char* id)
{
    return sl_responds_to2(vm, object, sl_cstring(vm, id));
}

SLVAL
sl_responds_to2(sl_vm_t* vm, SLVAL object, sl_string_t* id)
{
    sl_object_t* recvp = sl_get_ptr(object);
    SLVAL klass = sl_class_of(vm, object);
    sl_class_t* klassp = NULL;
    
    if(recvp->singleton_methods) {
        if(st_lookup(recvp->singleton_methods, (st_data_t)id, NULL)) {
            return vm->lib._true;
        }
    }
    
    klassp = (sl_class_t*)sl_get_ptr(klass);
    while(klassp->base.primitive_type != SL_T_NIL) {
        if(st_lookup(klassp->instance_methods, (st_data_t)id, NULL)) {
            return vm->lib._true;
        }
        klassp = (sl_class_t*)sl_get_ptr(klassp->super);
    }
    
    return vm->lib._false;
}

SLVAL
sl_get_ivar(sl_vm_t* vm, SLVAL object, sl_string_t* id)
{
    sl_object_t* p;
    SLVAL val;
    if(sl_is_a(vm, object, vm->lib.Int)) {
        return vm->lib.nil;
    }
    p = sl_get_ptr(object);
    if(st_lookup(p->instance_variables, (st_data_t)id, (st_data_t*)&val)) {
        return val;
    }
    return vm->lib.nil;
}

SLVAL
sl_get_cvar(sl_vm_t* vm, SLVAL object, sl_string_t* id)
{
    sl_class_t* p;
    SLVAL val;
    if(!sl_is_a(vm, object, vm->lib.Class)) {
        val = sl_class_of(vm, val);
    }
    p = (sl_class_t*)sl_get_ptr(object);
    if(st_lookup(p->class_variables, (st_data_t)id, (st_data_t*)&val)) {
        return val;
    }
    return vm->lib.nil;
}

void
sl_set_ivar(sl_vm_t* vm, SLVAL object, sl_string_t* id, SLVAL val)
{
    sl_object_t* p;
    if(sl_is_a(vm, object, vm->lib.Int)) {
        sl_throw_message2(vm, vm->lib.TypeError, "Can't set instance variable on Int");
    }
    p = sl_get_ptr(object);
    st_insert(p->instance_variables, (st_data_t)id, (st_data_t)sl_get_ptr(val));
}

void
sl_set_cvar(sl_vm_t* vm, SLVAL object, sl_string_t* id, SLVAL val)
{
    sl_class_t* p;
    if(!sl_is_a(vm, object, vm->lib.Class)) {
        val = sl_class_of(vm, val);
    }
    p = (sl_class_t*)sl_get_ptr(object);
    st_insert(p->class_variables, (st_data_t)id, (st_data_t)sl_get_ptr(val));
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
    return sl_send2(vm, recv, sl_make_cstring(vm, id), argc, argv);
}

static SLVAL
apply(sl_vm_t* vm, SLVAL recv, sl_method_t* method, size_t argc, SLVAL* argv)
{
    char errstr[128];
    sl_eval_ctx_t* ctx;
    size_t i;
    SLVAL arg;
    if(method->arity < 0) {
        if((size_t)(-method->arity - 1) > argc) {
            snprintf(errstr, 127, "Too few arguments. Expected %d, received %lu.", (-method->arity - 1), argc);
            sl_throw_message2(vm, vm->lib.ArgumentError, errstr);
        }
    } else {
        if((size_t)method->arity > argc) {
            snprintf(errstr, 127, "Too few arguments. Expected %d, received %lu.", method->arity, argc);
            sl_throw_message2(vm, vm->lib.ArgumentError, errstr);
        }
        if((size_t)method->arity < argc) {
            snprintf(errstr, 127, "Too many arguments. Expected %d, received %lu.", method->arity, argc);
            sl_throw_message2(vm, vm->lib.ArgumentError, errstr);
        }
    }
    if(method->is_c_func) {
        if(method->arity < 0) {
            return method->as.c.func(vm, recv, argc, argv);
        }
        switch(method->arity) {
            case 0: return method->as.c.func(vm, recv);
            case 1: return method->as.c.func(vm, recv, argv[0]);
            case 2: return method->as.c.func(vm, recv, argv[0], argv[1]);
            case 3: return method->as.c.func(vm, recv, argv[0], argv[1], argv[2]);
            case 4: return method->as.c.func(vm, recv, argv[0], argv[1], argv[2], argv[3]);
            default:
                sl_throw_message(vm, "Too many arguments for C function");
        }
    } else {
        ctx = sl_close_eval_ctx(vm, method->as.sl.ctx);
        ctx->self = recv;
        for(i = 0; i < method->as.sl.argc; i++) {
            arg = argv[i];
            st_insert(ctx->vars,
                (st_data_t)method->as.sl.argv[i], (st_data_t)sl_get_ptr(arg));
        }
        return method->as.sl.body->eval(method->as.sl.body, ctx);
    }
    return vm->lib.nil;
}

SLVAL
sl_send2(sl_vm_t* vm, SLVAL recv, SLVAL idv, size_t argc, SLVAL* argv)
{
    sl_method_t* method;
    SLVAL klass = sl_class_of(vm, recv);
    sl_class_t* klassp;
    sl_object_t* recvp = sl_get_ptr(recv);
    SLVAL* argv2;
    sl_string_t* id;
    SLVAL error;
    
    sl_expect(vm, idv, vm->lib.String);
    id = (sl_string_t*)sl_get_ptr(idv);
    
    if(sl_get_primitive_type(recv) != SL_T_INT && recvp->singleton_methods) {
        if(st_lookup(recvp->singleton_methods, (st_data_t)id, (st_data_t*)&method)) {
            return apply(vm, recv, method, argc, argv);
        }
    }
    
    klassp = (sl_class_t*)sl_get_ptr(klass);
    while(klassp->base.primitive_type != SL_T_NIL) {
        if(st_lookup(klassp->instance_methods, (st_data_t)id, (st_data_t*)&method)) {
            return apply(vm, recv, method, argc, argv);
        }
        klassp = (sl_class_t*)sl_get_ptr(klassp->super);
    }
    
    /* look for method_missing method */
    
    argv2 = alloca((argc + 1) * sizeof(SLVAL));
    memcpy(argv2 + 1, argv, sizeof(SLVAL) * argc);
    argv2[0] = sl_make_ptr((sl_object_t*)id);
    id = sl_cstring(vm, "method_missing");
    
    klassp = (sl_class_t*)sl_get_ptr(klass);
    while(klassp->base.primitive_type != SL_T_NIL) {
        if(st_lookup(klassp->instance_methods, (st_data_t)id, (st_data_t*)&method)) {
            return apply(vm, recv, method, argc, argv);
        }
        klassp = (sl_class_t*)sl_get_ptr(klassp->super);
    }
    
    /* nope */
    
    error = sl_make_cstring(vm, "Undefined method '");
    error = sl_string_concat(vm, error, idv);
    error = sl_string_concat(vm, error, sl_make_cstring(vm, "' on "));
    error = sl_string_concat(vm, error, sl_object_to_s(vm, recv));
    sl_throw(vm, sl_make_error2(vm, vm->lib.NoMethodError, error));
    return vm->lib.nil; /* shutup gcc */
}
