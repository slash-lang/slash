#include "slash/slash.h"
#include <string.h>
#include <stdio.h>

typedef struct {
    sl_object_t base;
    FILE* file;
}
sl_file_t;

static void
free_file(sl_file_t* file)
{
    if(file->file) {
        fclose(file->file);
    }
}

static sl_object_t*
allocate_file(sl_vm_t* vm)
{
    sl_file_t* file = sl_alloc(vm->arena, sizeof(sl_file_t));
    sl_gc_set_finalizer(vm->arena, file, (void(*)(void*))free_file);
    return (sl_object_t*)file;
}

static sl_file_t*
get_file(sl_vm_t* vm, SLVAL obj)
{
    return (sl_file_t*)sl_get_ptr(sl_expect(vm, obj, vm->lib.File));
}

static SLVAL
file_init(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    sl_file_t* file = get_file(vm, self);
    sl_string_t* str = (sl_string_t*)sl_get_ptr(argv[0]);
    char *m, *f, mode[8];
    SLVAL err;
    strcpy(mode, "r");
    if(argc > 1) {
        m = sl_to_cstr(vm, argv[1]);
        if(strcmp(mode, "r") && strcmp(mode, "w") && strcmp(mode, "a")
            && strcmp(mode, "r+") && strcmp(mode, "w+") && strcmp(mode, "a+")) {
            sl_throw_message2(vm, vm->lib.ArgumentError, "Invalid mode");
        }
        strcpy(mode, m);
    }
    strcat(mode, "b");
    if(strchr(mode, 'r') && memchr(str->buff, 0, str->buff_len)) {
        /* filename contains a null byte */
        err = sl_make_cstring(vm, "No such file: ");
        err = sl_string_concat(vm, err, argv[0]);
        sl_throw(vm, sl_make_error2(vm, sl_class_get_const(vm, vm->lib.File, "NotFound"), err));
    }
    f = sl_to_cstr(vm, argv[0]);
    f = sl_realpath(vm, f);
    file->file = fopen(f, mode);
    if(!file->file) {
        err = sl_make_cstring(vm, "No such file: ");
        err = sl_string_concat(vm, err, argv[0]);
        sl_throw(vm, sl_make_error2(vm, sl_class_get_const(vm, vm->lib.File, "NotFound"), err));
    }
    return self;
}

static SLVAL
file_write(sl_vm_t* vm, SLVAL self, SLVAL str)
{
    sl_file_t* file = get_file(vm, self);
    sl_string_t* strp = (sl_string_t*)sl_get_ptr(sl_to_s(vm, str));
    if(!file->file) {
        sl_throw_message2(vm, sl_class_get_const(vm, vm->lib.File, "InvalidOperation"), "Can't write to closed file");
    }
    return sl_make_int(vm, fwrite(strp->buff, 1, strp->buff_len, file->file));
}

void
sl_init_file(sl_vm_t* vm)
{
    vm->lib.File = sl_define_class(vm, "File", vm->lib.Object);
    sl_define_class3(vm, sl_make_cstring(vm, "NotFound"), vm->lib.Error, vm->lib.File);
    sl_define_class3(vm, sl_make_cstring(vm, "InvalidOperation"), vm->lib.Error, vm->lib.File);
    sl_class_set_allocator(vm, vm->lib.File, allocate_file);
    sl_define_method(vm, vm->lib.File, "init", -2, file_init);
    sl_define_method(vm, vm->lib.File, "write", 1, file_write);
}
