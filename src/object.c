#include <alloca.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "object.h"
#include "class.h"
#include "string.h"

void
sl_pre_init_object(sl_vm_t* vm)
{
    sl_class_t* klass;
    klass = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    klass->super = vm->lib.nil;
    klass->name = vm->lib.nil;
    klass->class_variables = st_init_table(&sl_string_hash_type);
    klass->class_methods = st_init_table(&sl_string_hash_type);
    klass->instance_methods = st_init_table(&sl_string_hash_type);
    klass->base.klass = vm->lib.Class;
    klass->base.primitive_type = SL_T_CLASS;
    klass->base.instance_variables = st_init_table(&sl_string_hash_type);
}

void
sl_init_object(sl_vm_t* vm)
{
    (void)vm;
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
    SLVAL* argv2;
    
    while(sl_get_primitive_type(klass) != SL_T_NIL) {
        klassp = (sl_class_t*)sl_get_ptr(klass);
        if(st_lookup(klassp->instance_methods, (st_data_t)id, (st_data_t*)&method)) {
            return apply(vm, recv, method, argc, argv);
        }
    }
    
    /* look for method_missing method */
    
    argv2 = alloca((argc + 1) * sizeof(SLVAL));
    memcpy(argv2 + 1, argv, argc);
    argv2[0] = sl_make_ptr((sl_object_t*)id);
    id = sl_cstring(vm, "method_missing");

    while(sl_get_primitive_type(klass) != SL_T_NIL) {
        klassp = (sl_class_t*)sl_get_ptr(klass);
        if(st_lookup(klassp->instance_methods, (st_data_t)id, (st_data_t*)&method)) {
            return apply(vm, recv, method, argc, argv);
        }
    }
    
    /* nope */
    
    sl_throw_message(vm, "Undefined method");
    return vm->lib.nil; /* shutup gcc */
}