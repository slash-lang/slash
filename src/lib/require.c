#include <slash.h>
#include <string.h>

static char*
suffixes[] = { "", ".sl" };

static sl_string_t*
resolve_require_path(sl_vm_t* vm, char* path)
{
    if(sl_abs_file_exists(path)) {
        return sl_cstring(vm, path);
    }
    SLVAL include_dirs = sl_class_get_const(vm, vm->lib.Object, "INC");
    sl_expect(vm, include_dirs, vm->lib.Array);
    size_t len = sl_get_int(sl_array_length(vm, include_dirs));
    SLVAL pathv = sl_make_cstring(vm, path);
    for(size_t inc_i = 0; inc_i < len; inc_i++) {
        for(size_t suff_i = 0; suff_i < sizeof(suffixes) / sizeof(*suffixes); suff_i++) {
            
            SLVAL full_path = sl_array_get(vm, include_dirs, inc_i);
            full_path = sl_string_concat(vm, full_path, sl_make_cstring(vm, "/"));
            full_path = sl_string_concat(vm, full_path, pathv);
            full_path = sl_string_concat(vm, full_path, sl_make_cstring(vm, suffixes[suff_i]));
            
            sl_string_t* spath = (sl_string_t*)sl_get_ptr(full_path);
            
            if(memchr(spath->buff, 0, spath->buff_len)) {
                continue;
            }
            
            if(sl_file_exists(vm, (char*)spath->buff)) {
                return spath;
            }
        }
    }
    return NULL;
}

SLVAL
sl_require(sl_vm_t* vm, SLVAL self, SLVAL file)
{
    sl_expect(vm, file, vm->lib.String);
    char* file_cstr = sl_to_cstr(vm, file);
    sl_string_t* resolved = resolve_require_path(vm, file_cstr);
    
    if(resolved) {
        SLVAL retn;
        if(st_lookup(vm->required, (st_data_t)resolved, (st_data_t*)&retn)) {
            return retn;
        }
        st_insert(vm->required, (st_data_t)resolved, (st_data_t)sl_get_ptr(vm->lib.nil));
        retn = sl_do_file(vm, (uint8_t*)sl_to_cstr(vm, sl_make_ptr((sl_object_t*)resolved)));
        st_insert(vm->required, (st_data_t)resolved, (st_data_t)sl_get_ptr(retn));
        return retn;
    }
    
    SLVAL err = sl_make_cstring(vm, "Could not load '");
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
    SLVAL inc[] = { sl_make_cstring(vm, ".") };
    sl_class_set_const(vm, vm->lib.Object, "INC", sl_make_array(vm, sizeof(inc) / sizeof(*inc), inc));
    sl_define_method(vm, vm->lib.Object, "require", 1, sl_require);
}
