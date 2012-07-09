#include <string.h>
#include <gc.h>
#include "value.h"
#include "vm.h"
#include "class.h"

static int
sl_string_hash(sl_string_t* str)
{
    uint32_t m = 0x5bd1e995;
    uint32_t r = 24;
    uint32_t seed = 0x9747b28c;
    
    int hash = seed ^ str->len;
    int k;
    size_t i;
    for(i = 0; i + 4 <= str->len; i += 4) {
        k = *(uint32_t*)&str->buff[i];
        
        k *= m;
        k ^= k >> r;
        k *= m;
        
        hash *= m;
        hash ^= k;
    }
    
    k = 0;
    for(; i < str->len; i++) {
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
    if(a->len < b->len) {
        return -1;
    } else if(a->len > b->len) {
        return 1;
    }
    return memcmp(a->buff, b->buff, a->len);
}

struct st_hash_type
sl_string_hash_type = { sl_string_cmp, sl_string_hash };

SLVAL
sl_make_string(sl_vm_t* vm, uint8_t* buff, size_t len)
{
    SLVAL vstr = sl_allocate(vm, vm->lib.String);
    sl_string_t* str = (sl_string_t*)sl_get_ptr(vstr);
    str->buff = GC_MALLOC_ATOMIC(len + 1);
    memcpy(str->buff, buff, len);
    str->buff[len] = 0;
    str->len = len;
    return vstr;
}

SLVAL
sl_make_cstring(struct sl_vm* vm, char* cstr)
{
    return sl_make_string(vm, (uint8_t*)cstr, strlen(cstr));
}

static sl_object_t*
sl_string_allocator()
{
    return (sl_object_t*)GC_MALLOC(sizeof(sl_object_t));
}

void
sl_init_string(sl_vm_t* vm)
{
    sl_class_t* klass;
    vm->lib.String = sl_define_class2(vm, vm->lib.nil, vm->lib.Object);
    klass = (sl_class_t*)sl_get_ptr(vm->lib.String);
    klass->allocator = sl_string_allocator;
}