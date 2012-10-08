#include <slash.h>
#include <string.h>

SLVAL
sl_require(sl_vm_t* vm, SLVAL self, SLVAL file)
{
    SLVAL include_dirs = sl_class_get_const(vm, vm->lib.Object, "INC");
    SLVAL err;
    char *canon_path;
    size_t i, len;
    sl_string_t* spath;
    sl_expect(vm, include_dirs, vm->lib.Array);
    sl_expect(vm, file, vm->lib.String);
    len = sl_get_int(sl_array_length(vm, include_dirs));
    for(i = 0; i < len; i++) {
        spath = (sl_string_t*)sl_get_ptr(
            sl_string_concat(vm, sl_array_get(vm, include_dirs, i),
                sl_string_concat(vm, sl_make_cstring(vm, "/"), file)));
        if(memchr(spath->buff, 0, spath->buff_len)) {
            /* path contains a NULL byte, ignore */
            continue;
        }
        if(sl_file_exists(vm, (char*)spath->buff)) {
            canon_path = sl_realpath(vm, (char*)spath->buff);
            return sl_do_file(vm, (uint8_t*)canon_path);
        }
    }
    err = sl_make_cstring(vm, "Could not load '");
    err = sl_string_concat(vm, err, file);
    err = sl_string_concat(vm, err, sl_make_cstring(vm, "'"));
    sl_throw(vm, sl_make_error2(vm, vm->lib.Error, err));
    (void)self;
    return vm->lib.nil;
}

void
sl_require_path_add(sl_vm_t* vm, char* path)
{
    SLVAL include_dirs = sl_class_get_const(vm, vm->lib.Object, "INC");
    if(!sl_is_a(vm, include_dirs, vm->lib.Array)) {
        return;
    }
    SLVAL pathv = sl_make_cstring(vm, path);
    sl_array_push(vm, include_dirs, 1, &pathv);
}

void
sl_init_require(sl_vm_t* vm)
{
    SLVAL inc[2];
    inc[0] = sl_make_cstring(vm, ".");
    inc[1] = sl_make_cstring(vm, "");
    sl_class_set_const(vm, vm->lib.Object, "INC", sl_make_array(vm, 2, inc));
    sl_define_method(vm, vm->lib.Object, "require", 1, sl_require);
}
