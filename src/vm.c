#include <gc.h>
#include "value.h"
#include "vm.h"
#include "string.h"
#include "class.h"
#include "object.h"
#include "error.h"

static int
sl_statically_initialized;

void
sl_static_init_exts();

void
sl_static_init()
{
    if(sl_statically_initialized) {
        return;
    }
    GC_INIT();
    sl_static_init_exts();
}

#define LIB(lib) void sl_init_##lib(sl_vm_t*)
#define LIB_INIT(lib) sl_init_##lib(vm)

LIB(method);

LIB(true);
LIB(false);
LIB(nil);
LIB(comparable);
LIB(number);
LIB(int);
LIB(float);
LIB(bignum);
LIB(method);
LIB(io);
LIB(enumerable);
LIB(array);
LIB(require);
LIB(lambda);
LIB(file);

void sl_init_exts(sl_vm_t* vm);

static void
sl_init_libs(sl_vm_t* vm)
{
    LIB_INIT(nil);
    LIB_INIT(comparable);
    LIB_INIT(number);
    LIB_INIT(int);
    LIB_INIT(float);
    LIB_INIT(bignum);
    LIB_INIT(true);
    LIB_INIT(false);
    LIB_INIT(io);
    LIB_INIT(enumerable);
    LIB_INIT(array);
    LIB_INIT(require);
    LIB_INIT(lambda);
    LIB_INIT(file);
    
    sl_init_exts(vm);
}

sl_vm_t*
sl_init()
{
    sl_vm_t* vm;
    sl_class_t* Object;
    sl_class_t* Class;
    sl_class_t* String;
    
    vm = GC_MALLOC(sizeof(sl_vm_t));
    vm->initializing = 1;
    
    vm->lib.nil = sl_make_ptr(GC_MALLOC(sizeof(sl_object_t)));
    vm->lib.Object = sl_make_ptr(GC_MALLOC(sizeof(sl_class_t)));
    
    sl_pre_init_class(vm);
    sl_pre_init_object(vm);
    sl_pre_init_string(vm);
    
    Object = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    Class = (sl_class_t*)sl_get_ptr(vm->lib.Class);
    String = (sl_class_t*)sl_get_ptr(vm->lib.String);
    Object->name = sl_make_cstring(vm, "Object");
    Class->name = sl_make_cstring(vm, "Class");
    String->name = sl_make_cstring(vm, "String");
    
    vm->initializing = 0;
    
    sl_init_method(vm);
    sl_init_class(vm);
    sl_init_object(vm);
    sl_init_string(vm);
    sl_init_error(vm);
    
    sl_init_libs(vm);
    
    String->super = vm->lib.Comparable;
    
    return vm;
}
