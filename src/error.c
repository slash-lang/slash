#include <stdio.h>
#include <stdlib.h>
#include <slash/lib/array.h>
#include <slash/lib/enumerable.h>
#include <slash/string.h>
#include <slash/error.h>
#include <slash/value.h>
#include <slash/class.h>
#include <slash/vm.h>

typedef struct sl_error {
    sl_object_t base;
    SLVAL message;
    SLVAL backtrace;
}
sl_error_t;

typedef struct sl_error_frame {
    sl_object_t base;
    SLVAL method;
    SLVAL file;
    SLVAL line;
}
sl_error_frame_t;

static sl_object_t*
allocate_error(sl_vm_t* vm)
{
    sl_error_t* err = sl_alloc(vm->arena, sizeof(sl_error_t));
    err->message = vm->lib.nil;
    err->backtrace = sl_make_array(vm, 0, NULL);
    return (sl_object_t*)err;
}

static sl_object_t*
allocate_error_frame(sl_vm_t* vm)
{
    sl_error_frame_t* frame = sl_alloc(vm->arena, sizeof(sl_error_frame_t));
    frame->method = vm->lib.nil;
    frame->file = vm->lib.nil;
    frame->line = vm->lib.nil;
    return (sl_object_t*)frame;
}

static sl_error_t*
get_error(sl_vm_t* vm, SLVAL object)
{
    sl_expect(vm, object, vm->lib.Error);
    return (sl_error_t*)sl_get_ptr(object);
}

static sl_error_frame_t*
get_error_frame(sl_vm_t* vm, SLVAL object)
{
    sl_expect(vm, object, vm->lib.Error_Frame);
    return (sl_error_frame_t*)sl_get_ptr(object);
}

static SLVAL
sl_error_init(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    sl_error_t* err = get_error(vm, self);
    if(argc > 0) {
        err->message = sl_expect(vm, argv[0], vm->lib.String);
    }
    return self;
}

static SLVAL
sl_error_name(sl_vm_t* vm, SLVAL self)
{
    return sl_to_s(vm, sl_class_of(vm, self));
}

static SLVAL
sl_error_message(sl_vm_t* vm, SLVAL self)
{
    sl_error_t* error = get_error(vm, self);
    return error->message;
}

static SLVAL
sl_error_backtrace(sl_vm_t* vm, SLVAL self)
{
    sl_error_t* error = get_error(vm, self);
    return error->backtrace;
}

static SLVAL
sl_error_to_s(sl_vm_t* vm, SLVAL self)
{
    sl_error_t* error = get_error(vm, self);
    SLVAL str = sl_error_name(vm, self);
    SLVAL nl = sl_make_cstring(vm, "\n");
    if(sl_is_a(vm, error->message, vm->lib.String)) {
        str = sl_string_concat(vm, str, sl_make_cstring(vm, ": "));
        str = sl_string_concat(vm, str, error->message);
    }
    str = sl_string_concat(vm, str, nl);
    str = sl_string_concat(vm, str, sl_enumerable_join(vm, error->backtrace, 1, &nl));
    return str;
}

static SLVAL
sl_error_frame_method(sl_vm_t* vm, SLVAL self)
{
    return get_error_frame(vm, self)->method;
}

static SLVAL
sl_error_frame_file(sl_vm_t* vm, SLVAL self)
{
    return get_error_frame(vm, self)->file;
}

static SLVAL
sl_error_frame_line(sl_vm_t* vm, SLVAL self)
{
    return get_error_frame(vm, self)->line;
}

static SLVAL
sl_error_frame_to_s(sl_vm_t* vm, SLVAL self)
{
    sl_error_frame_t* frame = get_error_frame(vm, self);
    SLVAL str = sl_make_cstring(vm, "  at ");
    str = sl_string_concat(vm, str, frame->method);
    str = sl_string_concat(vm, str, sl_make_cstring(vm, " in "));
    str = sl_string_concat(vm, str, frame->file);
    str = sl_string_concat(vm, str, sl_make_cstring(vm, ", line "));
    str = sl_string_concat(vm, str, sl_to_s_no_throw(vm, frame->line));
    return str;
}

void
sl_error_add_frame(struct sl_vm* vm, SLVAL error, SLVAL method, SLVAL file, SLVAL line)
{
    sl_error_t* e = get_error(vm, error);
    
    SLVAL frv = sl_allocate(vm, vm->lib.Error_Frame);
    sl_error_frame_t* frame = (sl_error_frame_t*)sl_get_ptr(frv);
    frame->method = sl_is_a(vm, method, vm->lib.String) ? method : sl_make_cstring(vm, "");
    frame->file = sl_is_a(vm, file, vm->lib.String) ? file : vm->lib.nil;
    frame->line = sl_is_a(vm, line, vm->lib.Int) ? line : vm->lib.nil;
    
    if(sl_is_truthy(file) && sl_is_truthy(line)) {
        SLVAL* prev_frames;
        size_t prev_frames_count = sl_array_items_no_copy(vm, e->backtrace, &prev_frames);
        for(size_t i = 0; i < prev_frames_count; i++) {
            sl_error_frame_t* prev_frame = (sl_error_frame_t*)sl_get_ptr(prev_frames[prev_frames_count - i - 1]);
            if(sl_is_truthy(prev_frame->file) && sl_is_truthy(prev_frame->line)) {
                break;
            }
            prev_frame->file = frame->file;
            prev_frame->line = frame->line;
        }
    }
    
    sl_array_push(vm, e->backtrace, 1, &frv);
}

void
sl_init_error(sl_vm_t* vm)
{
    vm->lib.Error = sl_define_class(vm, "Error", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.Error, allocate_error);
    sl_define_method(vm, vm->lib.Error, "init", -1, sl_error_init);
    sl_define_method(vm, vm->lib.Error, "name", 0, sl_error_name);
    sl_define_method(vm, vm->lib.Error, "message", 0, sl_error_message);
    sl_define_method(vm, vm->lib.Error, "backtrace", 0, sl_error_backtrace);
    sl_define_method(vm, vm->lib.Error, "to_s", 0, sl_error_to_s);
    
    vm->lib.Error_Frame = sl_define_class3(vm, sl_make_cstring(vm, "Frame"), vm->lib.Object, vm->lib.Error);
    sl_class_set_allocator(vm, vm->lib.Error_Frame, allocate_error_frame);
    sl_define_method(vm, vm->lib.Error_Frame, "method", 0, sl_error_frame_method);
    sl_define_method(vm, vm->lib.Error_Frame, "file", 0, sl_error_frame_file);
    sl_define_method(vm, vm->lib.Error_Frame, "line", 0, sl_error_frame_line);
    sl_define_method(vm, vm->lib.Error_Frame, "to_s", 0, sl_error_frame_to_s);
    
    #define ERROR(klass) vm->lib.klass = sl_define_class(vm, #klass, vm->lib.Error)
    ERROR(ArgumentError);
    ERROR(EncodingError);
    ERROR(NameError);
    ERROR(NoMethodError);
    ERROR(StackOverflowError);
    ERROR(SyntaxError);
    ERROR(TypeError);
    ERROR(ZeroDivisionError);
    ERROR(NotImplementedError);
    #undef ERROR
}

SLVAL
sl_make_error(sl_vm_t* vm, SLVAL message)
{
    return sl_make_error2(vm, vm->lib.Error, message);
}

SLVAL
sl_make_error2(sl_vm_t* vm, SLVAL klass, SLVAL message)
{
    SLVAL err = sl_allocate(vm, klass);
    sl_expect(vm, message, vm->lib.String);
    get_error(vm, err)->message = message;
    return err;
}

void
sl_unwind(sl_vm_t* vm, SLVAL value, sl_unwind_type_t type)
{
    if(vm->catch_stack == NULL) {
        fprintf(stderr, "Attempting to unwind stack with no handler\n");
        abort();
    }
    vm->catch_stack->value = value;
    vm->catch_stack->type = type;
    sl_longjmp(vm->catch_stack->env, 1);
}

void
sl_throw(sl_vm_t* vm, SLVAL error)
{
    sl_unwind(vm, error, SL_UNWIND_EXCEPTION);
}

void
sl_exit(sl_vm_t* vm, SLVAL value)
{
    sl_unwind(vm, sl_expect(vm, value, vm->lib.Int), SL_UNWIND_EXIT);
}

void
sl_rethrow(struct sl_vm* vm, sl_catch_frame_t* frame)
{
    sl_unwind(vm, frame->value, frame->type);
}

void
sl_throw_message(struct sl_vm* vm, char* cstr)
{
    sl_throw(vm, sl_make_error(vm, sl_make_cstring(vm, cstr)));
}

void
sl_throw_message2(struct sl_vm* vm, SLVAL klass, char* cstr)
{
    sl_throw(vm, sl_make_error2(vm, klass, sl_make_cstring(vm, cstr)));
}
