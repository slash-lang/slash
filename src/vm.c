#include <gc.h>
#include <stdlib.h>
#include <time.h>
#include "lib/rand.h"
#include "value.h"
#include "vm.h"
#include "string.h"
#include "class.h"
#include "object.h"
#include "error.h"
#include "platform.h"

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
    sl_statically_initialized = 1;
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
LIB(enumerable);
LIB(array);
LIB(require);
LIB(lambda);
LIB(file);
LIB(dict);
LIB(rand);
LIB(request);
LIB(response);
LIB(system);
LIB(regexp);

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
    LIB_INIT(enumerable);
    LIB_INIT(array);
    LIB_INIT(require);
    LIB_INIT(lambda);
    LIB_INIT(file);
    LIB_INIT(dict);
    LIB_INIT(rand);
    LIB_INIT(request);
    LIB_INIT(response);
    LIB_INIT(system);
    LIB_INIT(regexp);
    
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
    vm->cwd = ".";
    vm->initializing = 1;
    vm->store = st_init_numtable();
    sl_rand_init_mt(vm);
    vm->hash_seed = sl_rand(vm);
    vm->stack_limit = sl_stack_limit();
    
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

SLVAL
sl_vm_store_get(sl_vm_t* vm, void* key)
{
    SLVAL val = vm->lib.nil;
    st_lookup(vm->store, (st_data_t)key, (st_data_t*)&val);
    return val;
}

void
sl_vm_store_put(sl_vm_t* vm, void* key, SLVAL val)
{
    st_insert(vm->store, (st_data_t)key, (st_data_t)sl_get_ptr(val));
}
