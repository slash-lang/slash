#include <slash/data.h>
#include <slash/method.h>

static void
free_data(void* data_)
{
    sl_data_t* data = data_;
    data->type->free(data->vm, data->data);
}

sl_object_t*
sl_make_data(sl_vm_t* vm, SLVAL klass, sl_data_type_t* type, void* data)
{
    sl_data_t* object = sl_alloc(vm->arena, sizeof(sl_data_t));
    object->base.klass = klass;
    object->base.primitive_type = SL_T_DATA;
    object->base.instance_variables = sl_st_init_table(vm, &sl_id_hash_type);
    object->type = type;
    object->data = data;
    object->vm = vm;
    sl_gc_set_finalizer(object, free_data);
    return (sl_object_t*)object;
}

static sl_data_t*
validate_data(sl_vm_t* vm, sl_data_type_t* type, SLVAL object)
{
    if(sl_get_primitive_type(object) != SL_T_DATA) {
        sl_error(vm, vm->lib.TypeError, "Expected %s object", type->name);
    }

    sl_data_t* data = (sl_data_t*)sl_get_ptr(object);

    if(data->type != type) {
        sl_error(vm, vm->lib.TypeError, "Expected %s object", type->name);
    }

    return data;
}

void*
sl_data_get_ptr(sl_vm_t* vm, sl_data_type_t* type, SLVAL object)
{
    return validate_data(vm, type, object)->data;
}

void
sl_data_set_ptr(sl_vm_t* vm, sl_data_type_t* type, SLVAL object, void* data)
{
    validate_data(vm, type, object)->data = data;
}
