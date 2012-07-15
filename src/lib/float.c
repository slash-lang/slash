#include <gc.h>
#include "class.h"
#include "value.h"
#include "lib/float.h"

typedef struct sl_float {
    sl_object_t base;
    double d;
}
sl_float_t;

static sl_object_t*
allocate_float()
{
    sl_object_t* obj = (sl_object_t*)GC_MALLOC(sizeof(sl_float_t));
    obj->primitive_type = SL_T_FLOAT;
    return obj;
}

static sl_float_t*
get_float(sl_vm_t* vm, SLVAL val)
{
    sl_expect(vm, val, vm->lib.Float);
    return (sl_float_t*)sl_get_ptr(val);
}

void
sl_init_float(sl_vm_t* vm)
{
    vm->lib.Float = sl_define_class(vm, "Float", vm->lib.Number);
    sl_class_set_allocator(vm, vm->lib.Float, allocate_float);
}


SLVAL
sl_make_float(sl_vm_t* vm, double d)
{
    SLVAL fl = sl_allocate(vm, vm->lib.Float);
    get_float(vm, fl)->d = d;
    return fl;
}

double
sl_get_float(sl_vm_t* vm, SLVAL f)
{
    return get_float(vm, f)->d;
}
