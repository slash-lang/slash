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
    klass->class_variables = st_init_table(&sl_string_hash_type);
    klass->instance_methods = st_init_table(&sl_string_hash_type);
    klass->base.klass = vm->lib.Class;
    klass->base.primitive_type = SL_T_CLASS;
    klass->base.instance_variables = st_init_table(&sl_string_hash_type);
}

void
sl_init_object(sl_vm_t* vm)
{
    sl_define_method(vm, vm->lib.Object, "to_s", 0, sl_object_to_s);
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
    if(!sl_get_ptr(object)->singleton_methods) {
        sl_get_ptr(object)->singleton_methods = st_init_table(&sl_string_hash_type);
    }
    name = sl_to_s(vm, name);
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
    sl_class_t* klassp = (sl_class_t*)sl_get_ptr(klass);
    
    if(recvp->singleton_methods) {
        if(st_lookup(recvp->singleton_methods, (st_data_t)id, NULL)) {
            return vm->lib.true;
        }
    }
    
    klassp = (sl_class_t*)sl_get_ptr(klass);
    while(klassp->base.primitive_type != SL_T_NIL) {
        if(st_lookup(klassp->instance_methods, (st_data_t)id, NULL)) {
            return vm->lib.true;
        }
        klassp = (sl_class_t*)sl_get_ptr(klassp->super);
    }
    
    return vm->lib.false;
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
    return sl_send2(vm, recv, sl_cstring(vm, id), argc, argv);
}

static SLVAL
apply(sl_vm_t* vm, SLVAL recv, sl_method_t* method, size_t argc, SLVAL* argv)
{
    if(method->arity < 0) {
        if((size_t)(-method->arity - 1) > argc) {
            sl_throw_message(vm, "Too few arguments. Expected %d, received %d.");
        }
    } else {
        if((size_t)method->arity > argc) {
            sl_throw_message(vm, "Too few arguments. Expected %d, received %d.");
        }
        if((size_t)method->arity < argc) {
            sl_throw_message(vm, "Too many arguments. Expected %d, received %d.");
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
        /* wat @TODO */
        abort();
    }
    return vm->lib.nil;
}

SLVAL
sl_send2(sl_vm_t* vm, SLVAL recv, sl_string_t* id, size_t argc, SLVAL* argv)
{
    sl_method_t* method;
    SLVAL klass = sl_class_of(vm, recv);
    sl_class_t* klassp;
    sl_object_t* recvp = sl_get_ptr(recv);
    SLVAL* argv2;
    
    if(recvp->singleton_methods) {
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
    memcpy(argv2 + 1, argv, argc);
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
    
    sl_throw_message(vm, "Undefined method");
    return vm->lib.nil; /* shutup gcc */
}
