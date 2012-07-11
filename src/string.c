#include <string.h>
#include <gc.h>
#include "string.h"
#include "value.h"
#include "vm.h"
#include "class.h"

static int
sl_string_hash(sl_string_t* str)
{
    uint32_t m = 0x5bd1e995;
    uint32_t r = 24;
    uint32_t seed = 0x9747b28c;
    
    int hash = seed ^ str->buff_len;
    int k;
    size_t i;
    for(i = 0; i + 4 <= str->buff_len; i += 4) {
        k = *(uint32_t*)&str->buff[i];
        
        k *= m;
        k ^= k >> r;
        k *= m;
        
        hash *= m;
        hash ^= k;
    }
    
    k = 0;
    for(; i < str->buff_len; i++) {
        k *= 256;
        k += str->buff[i];
    }
    hash ^= k;
    hash *= m;
    
    hash ^= hash >> 13;
    hash *= m;
    hash ^= hash >> 15;
    
    return hash;
}

static int
sl_string_cmp(sl_string_t* a, sl_string_t* b)
{
    if(a->buff_len < b->buff_len) {
        return -1;
    } else if(a->buff_len > b->buff_len) {
        return 1;
    }
    return memcmp(a->buff, b->buff, a->buff_len);
}

struct st_hash_type
sl_string_hash_type = { sl_string_cmp, sl_string_hash };

SLVAL
sl_make_string(sl_vm_t* vm, uint8_t* buff, size_t buff_len)
{
    SLVAL vstr = sl_allocate(vm, vm->lib.String);
    sl_string_t* str = (sl_string_t*)sl_get_ptr(vstr);
    str->char_len = sl_utf8_strlen(vm, buff, buff_len);
    str->buff = GC_MALLOC_ATOMIC(buff_len + 1);
    memcpy(str->buff, buff, buff_len);
    str->buff[buff_len] = 0;
    str->buff_len = buff_len;
    return vstr;
}

SLVAL
sl_make_cstring(struct sl_vm* vm, char* cstr)
{
    return sl_make_string(vm, (uint8_t*)cstr, strlen(cstr));
}

sl_string_t*
sl_cstring(struct sl_vm* vm, char* cstr)
{
    return (sl_string_t*)sl_get_ptr(sl_make_cstring(vm, cstr));
}

static sl_object_t*
allocate_string()
{
    return (sl_object_t*)GC_MALLOC(sizeof(sl_object_t));
}

static uint32_t utf8_code_point_limit[] = {
    0,
    0x000080,
    0x000800,
    0x010000,
    0x200000
};

size_t
sl_utf8_strlen(struct sl_vm* vm, uint8_t* buff, size_t len)
{
    size_t i, bytes, j, utf8len = 0;
    uint8_t c;
    uint32_t c32;
    for(i = 0; i < len; i++) {
        c = buff[i];
        if(c & 0x80) {
            if((c & 0xe0) == 0xc0) {
                c32 = c & 0x1f;
                bytes = 2;
            } else if((c & 0xf0) == 0xe0) {
                c32 = c & 0x0f;
                bytes = 3;
            } else if((c & 0xf8) == 0xf0) {
                c32 = c & 0x07;
                bytes = 4;
            } else {
                sl_throw(vm, sl_make_error2(vm, vm->lib.SyntaxError /* @TODO maybe EncodingError? */, sl_make_cstring(vm, "Invalid UTF-8 sequence")));
            }
        } else {
            c32 = c;
            bytes = 1;
        }
        if(i + bytes > len) {
            sl_throw(vm, sl_make_error2(vm, vm->lib.SyntaxError /* @TODO maybe EncodingError? */, sl_make_cstring(vm, "Invalid UTF-8 sequence")));
        }
        for(j = 1; j < bytes; j++) {
            c32 <<= 6;
            c32 |= buff[++i] & 0x3f;
        }
        if(c32 < utf8_code_point_limit[bytes - 1]) {
            sl_throw(vm, sl_make_error2(vm, vm->lib.SyntaxError /* @TODO maybe EncodingError? */, sl_make_cstring(vm, "Invalid UTF-8 sequence")));
        }
        utf8len++;
    }
    return utf8len;
}

static sl_string_t*
get_string(sl_vm_t* vm, SLVAL obj)
{
    sl_expect(vm, obj, vm->lib.String);
    return (sl_string_t*)sl_get_ptr(obj);
}

SLVAL
sl_string_length(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, get_string(vm, self)->char_len);
}

void
sl_pre_init_string(sl_vm_t* vm)
{
    vm->lib.String = sl_define_class2(vm, vm->lib.nil, vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.String, allocate_string);
}

void
sl_init_string(sl_vm_t* vm)
{
    sl_define_method(vm, vm->lib.String, "length", 0, sl_string_length);
}