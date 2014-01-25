#include <slash.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

typedef struct {
    sl_object_t base;
    FILE* file;
}
sl_file_t;

static void
free_file(void* ptr)
{
    sl_file_t* file = ptr;

    if(file->file) {
        fclose(file->file);
    }
}

static sl_gc_shape_t
file_shape = {
    .mark     = sl_gc_conservative_mark,
    .finalize = free_file,
};

static sl_object_t*
allocate_file(sl_vm_t* vm)
{
    return sl_alloc2(vm->arena, &file_shape, sizeof(sl_file_t));
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
    sl_string_t* str = sl_get_string(vm, argv[0]);
    char *m, *f, mode[8];
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
        sl_error(vm, vm->lib.File_NotFound, "No such file: %V", argv[0]);
    }
    f = sl_to_cstr(vm, argv[0]);
    f = sl_realpath(vm, f);
    file->file = fopen(f, mode);
    if(!file->file) {
        sl_error(vm, vm->lib.File_NotFound, "Couldn't open: %V - %s", argv[0], strerror(errno));
    }
    return self;
}

static SLVAL
file_write(sl_vm_t* vm, SLVAL self, SLVAL str)
{
    sl_file_t* file = get_file(vm, self);
    sl_string_t* strp = (sl_string_t*)sl_get_ptr(sl_to_s(vm, str));
    if(!file->file) {
        sl_throw_message2(vm, vm->lib.File_InvalidOperation, "Can't write to closed file");
    }
    return sl_make_int(vm, fwrite(strp->buff, 1, strp->buff_len, file->file));
}

static SLVAL
file_read(sl_vm_t* vm, SLVAL self)
{
    sl_file_t* file = get_file(vm, self);
    if(!file->file) {
        sl_throw_message2(vm, vm->lib.File_InvalidOperation, "Can't write to closed file");
    }
    uint8_t* buff = sl_alloc_buffer(vm->arena, 4096);
    size_t buff_cap = 4096, buff_len = 0;
    while(!feof(file->file)) {
        int read = fread(buff + buff_len, 1, 4096, file->file);
        buff_len += read;
        if(buff_len + 4096 >= buff_cap) {
            buff_cap *= 2;
            buff = sl_realloc(vm->arena, buff, buff_cap);
        }
    }
    return sl_make_string(vm, buff, buff_len);
}

static SLVAL
file_close(sl_vm_t* vm, SLVAL self)
{
    sl_file_t* file = get_file(vm, self);
    if(file->file) {
        fclose(file->file);
        file->file = NULL;
    }
    return vm->lib.nil;
}

static SLVAL
file_closed(sl_vm_t* vm, SLVAL self)
{
    sl_file_t* file = get_file(vm, self);
    return sl_make_bool(vm, file->file == NULL);
}

static SLVAL
file_class_read(sl_vm_t* vm, SLVAL self, SLVAL filename)
{
    SLVAL f = sl_new(vm, vm->lib.File, 1, &filename);
    sl_vm_frame_t frame;
    volatile SLVAL retn;
    SL_ENSURE(frame, {
        retn = file_read(vm, f);
    }, {
        file_close(vm, f);
    });
    return retn;
    (void)self; /* unused */
}

static SLVAL
file_class_exists(sl_vm_t* vm, SLVAL self, SLVAL filename)
{
    sl_string_t* str = sl_get_string(vm, filename);
    if(memchr(str->buff, 0, str->buff_len)) {
        return vm->lib._false;
    }
    return sl_make_bool(vm, sl_file_exists(vm, (char*)str->buff));
    (void)self;
}

void
sl_init_file(sl_vm_t* vm)
{
    vm->lib.File = sl_define_class(vm, "File", vm->lib.Object);
    vm->lib.File_NotFound = sl_define_class3(vm, sl_intern(vm, "NotFound"), vm->lib.Error, vm->lib.File);
    vm->lib.File_InvalidOperation = sl_define_class3(vm, sl_intern(vm, "InvalidOperation"), vm->lib.Error, vm->lib.File);
    sl_class_set_allocator(vm, vm->lib.File, allocate_file);
    sl_define_method(vm, vm->lib.File, "init", -2, file_init);
    sl_define_method(vm, vm->lib.File, "write", 1, file_write);
    sl_define_method(vm, vm->lib.File, "read", 0, file_read);
    sl_define_method(vm, vm->lib.File, "close", 0, file_close);
    sl_define_method(vm, vm->lib.File, "closed", 0, file_closed);
    /* class convenience methods: */
    sl_define_singleton_method(vm, vm->lib.File, "read", 1, file_class_read);
    sl_define_singleton_method(vm, vm->lib.File, "exists", 1, file_class_exists);
}
