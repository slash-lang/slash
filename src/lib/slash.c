#include <slash.h>
#include <slash/dispatch.h>

typedef struct {
    sl_object_t base;
    sl_vm_t* vm;
    sl_vm_t* host_vm;
    SLVAL output_handler;
}
slash_t;

static sl_vm_t*
get_vm(sl_vm_t* vm, SLVAL slash)
{
    slash_t* sl = (slash_t*)sl_get_ptr(slash);
    if(!sl->vm) {
        sl_error(vm, vm->lib.TypeError, "Invalid operation on uninitialized Slash");
    }
    return sl->vm;
}

static void
slash_free(void* obj)
{
    slash_t* sl = obj;
    if(sl->vm) {
        sl_free_gc_arena(sl->vm->arena);
    }
}

static sl_object_t*
slash_alloc(sl_vm_t* vm)
{
    slash_t* sl = sl_alloc(vm->arena, sizeof(*sl));
    sl->vm = NULL;
    sl->host_vm = vm;
    sl->output_handler = vm->lib.nil;
    sl_gc_set_finalizer(sl, slash_free);
    return (sl_object_t*)sl;
}

static void
output(sl_vm_t* sub_vm, char* buff, size_t len)
{
    slash_t* sl = sub_vm->data;
    sl_vm_t* vm = sl->host_vm;
    if(sl_responds_to2(vm, sl->output_handler, vm->id.call)) {
        SLVAL ex;
        sl_vm_frame_t catch_frame;
        SL_TRY(catch_frame, SL_UNWIND_ALL, {
            SLVAL string = sl_make_string(vm, (uint8_t*)buff, len);
            sl_send_id(vm, sl->output_handler, vm->id.call, 1, string);
        }, ex, {
            /* silently ignore */
        });
    }
}

static SLVAL
slash_init(sl_vm_t* vm, SLVAL slash, SLVAL sapi_name)
{
    slash_t* sl = (slash_t*)sl_get_ptr(slash);
    if(sl->vm) {
        sl_free_gc_arena(sl->vm->arena);
        sl->vm = NULL;
    }
    char* sapi_name_str = sl_to_cstr(vm, sl_expect(vm, sapi_name, vm->lib.String));

    sl->vm = sl_init(sapi_name_str);
    sl->host_vm = vm;
    sl->vm->data = sl;

    sl_gc_set_stack_top(sl->vm->arena, sl_gc_get_stack_top(vm->arena));

    sl_response_opts_t response_opts;
    response_opts.descriptive_error_pages = 0;
    response_opts.buffered = 0;
    response_opts.write = output;
    sl_response_set_opts(sl->vm, &response_opts);

    return vm->lib.nil;
}

static SLVAL
slash_eval(sl_vm_t* host_vm, SLVAL slash, SLVAL code_value)
{
    /* TODO - need some way to dup objects between VMs */
    sl_string_t* code = sl_get_string(host_vm, code_value);
    sl_vm_t* vm = get_vm(host_vm, slash);

    sl_vm_frame_t catch_frame;
    SLVAL ex, retval = host_vm->lib._true;
    SL_TRY(catch_frame, SL_UNWIND_ALL, {
        sl_do_string(vm, code->buff, code->buff_len, "(eval)", 1);  
    }, ex, {
        retval = host_vm->lib._false;
    });
    return retval;
}

static SLVAL
slash_output_handler(sl_vm_t* vm, SLVAL slash)
{
    slash_t* sl = (slash_t*)sl_get_ptr(slash);
    return sl->output_handler;
    (void)vm;
}

static SLVAL
slash_output_handler_set(sl_vm_t* vm, SLVAL slash, SLVAL handler)
{
    slash_t* sl = (slash_t*)sl_get_ptr(slash);
    return sl->output_handler = handler;
    (void)vm;
}

void
sl_init_slash(sl_vm_t* vm)
{
    SLVAL Slash = sl_define_class(vm, "Slash", vm->lib.Object);
    sl_class_set_allocator(vm, Slash, slash_alloc);
    sl_define_method(vm, Slash, "init", 1, slash_init);
    sl_define_method(vm, Slash, "eval", 1, slash_eval);
    sl_define_method(vm, Slash, "[]", 1, slash_eval);
    sl_define_method(vm, Slash, "output_handler", 0, slash_output_handler);
    sl_define_method(vm, Slash, "output_handler=", 1, slash_output_handler_set);
    sl_class_set_const(vm, Slash, "VERSION", sl_make_cstring(vm, SL_VERSION));
}
