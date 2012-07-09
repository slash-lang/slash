#include <gc.h>
#include "value.h"
#include "vm.h"
#include "string.h"
#include "class.h"
#include "object.h"

static int
sl_statically_initialized;

void
sl_static_init()
{
    if(sl_statically_initialized) {
        return;
    }
    GC_INIT();
}

sl_vm_t*
sl_init()
{
    sl_vm_t* vm;
    sl_class_t* Object;
    sl_class_t* Class;
    sl_class_t* String;
    sl_static_init();
    vm = GC_MALLOC(sizeof(sl_vm_t));
    vm->initializing = 1;
    
    vm->globals = st_init_table(&sl_string_hash_type);
    
    vm->lib.nil = sl_make_ptr(GC_MALLOC(sizeof(sl_object_t)));
    vm->lib.Object = sl_make_ptr(GC_MALLOC(sizeof(sl_class_t)));
    
    sl_init_class(vm);
    sl_init_object(vm);
    sl_init_string(vm);
    
    Object = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    Class = (sl_class_t*)sl_get_ptr(vm->lib.Class);
    Object->name = sl_make_cstring(vm, "Object");
    Class->name = sl_make_cstring(vm, "Class");
    String->name = sl_make_cstring(vm, "String");
    
    vm->initializing = 0;
    
    return vm;
}