#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iconv.h>
#include <errno.h>
#include <slash/object.h>
#include <slash/string.h>
#include <slash/lib/buffer.h>
#include <slash/lib/bignum.h>

typedef struct {
    sl_object_t base;
    char* buffer;
    size_t len;
    size_t cap;
}
sl_buffer_t;

static sl_object_t*
allocate_buffer(sl_vm_t* vm)
{
    sl_buffer_t* buf = sl_alloc(vm->arena, sizeof(sl_buffer_t));
    buf->cap = 16;
    buf->len = 0;
    buf->buffer = sl_alloc_buffer(vm->arena, buf->cap);
    return (sl_object_t*)buf;
}

static sl_buffer_t*
get_buffer(sl_vm_t* vm, SLVAL obj)
{
    sl_expect(vm, obj, vm->lib.Buffer);
    return (sl_buffer_t*)sl_get_ptr(obj);
}

static void
ensure_buffer(sl_vm_t* vm, sl_buffer_t* buff)
{
    if(sl_unlikely(!buff->buffer)) {
        sl_throw_message2(vm, vm->lib.TypeError, "Invalid operation on uninitialized Buffer");
    }
}

static long
translate_index(sl_buffer_t* buff, long index)
{
    if(index < 0) {
        index += buff->len;
    }
    if(index < 0 || (size_t /* index >= 0 at this point */)index >= buff->len) {
        return -1;
    }
    return index;
}

SLVAL
sl_make_buffer(sl_vm_t* vm, char* data, size_t len)
{
    return sl_make_buffer2(vm, data, len, len);
}

SLVAL
sl_make_buffer2(sl_vm_t* vm, char* data, size_t len, size_t capacity)
{
    SLVAL bufferv = sl_allocate(vm, vm->lib.Buffer);
    sl_buffer_t* buff = get_buffer(vm, bufferv);
    buff->len = len;
    buff->cap = capacity;
    buff->buffer = sl_alloc_buffer(vm->arena, capacity);
    memcpy(buff->buffer, data, capacity);
    return bufferv;
}

static SLVAL
buffer_init(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    if(argc > 0) {
        sl_buffer_t* buff = get_buffer(vm, self);
        if(SL_IS_INT(argv[0])) {
            long cap = sl_get_int(argv[0]);
            if(cap < 0) {
                sl_throw_message2(vm, vm->lib.ArgumentError, "Initial buffer capacity must be positive");
            }
            if(cap < 4) {
                cap = 4;
            }
            buff->cap = cap;
            buff->len = 0;
            buff->buffer = realloc(buff->buffer, cap);
        } else {
            /* init with string */
            sl_expect(vm, argv[0], vm->lib.String);
            sl_string_t* str = (sl_string_t*)sl_get_ptr(argv[0]);
            buff->len = str->buff_len;
            buff->cap = str->buff_len;
            buff->buffer = realloc(buff->buffer, buff->cap);
            memcpy(buff->buffer, str->buff, buff->len);
        }
    }
    return self;
}

static SLVAL
buffer_get(sl_vm_t* vm, SLVAL self, SLVAL indexv)
{
    sl_buffer_t* buff = get_buffer(vm, self);
    ensure_buffer(vm, buff);
    if(sl_likely(SL_IS_INT(indexv))) {
        long index = translate_index(buff, sl_get_int(indexv));
        if(index < 0) {
            return vm->lib.nil;
        } else {
            return sl_make_int(vm, buff->buffer[index]);
        }
    }
    if(sl_is_a(vm, indexv, vm->lib.Bignum)) {
        long index = translate_index(buff, sl_bignum_get_long(vm, indexv));
        if(index < 0) {
            return vm->lib.nil;
        } else {
            return sl_make_int(vm, buff->buffer[index]);
        }
    }
    /* TODO index by range */
    sl_expect(vm, indexv, vm->lib.Int);
    return vm->lib.nil;
}

static SLVAL
buffer_set(sl_vm_t* vm, SLVAL self, SLVAL indexv, SLVAL bytev)
{
    sl_buffer_t* buff = get_buffer(vm, self);
    ensure_buffer(vm, buff);
    uint8_t byte = (uint8_t)sl_get_int(sl_expect(vm, bytev, vm->lib.Int));
    if(sl_likely(SL_IS_INT(indexv))) {
        long index = translate_index(buff, sl_get_int(indexv));
        if(index < 0) {
            sl_throw_message2(vm, vm->lib.ArgumentError, "index out of range");
        } else {
            buff->buffer[index] = byte;
            return bytev;
        }
    }
    if(sl_is_a(vm, indexv, vm->lib.Bignum)) {
        long index = translate_index(buff, sl_bignum_get_long(vm, indexv));
        if(index < 0) {
            sl_throw_message2(vm, vm->lib.ArgumentError, "index out of range");
        } else {
            buff->buffer[index] = byte;
            return bytev;
        }
    }
    /* TODO index by range */
    sl_expect(vm, indexv, vm->lib.Int);
    return vm->lib.nil;
}

static SLVAL
buffer_length(sl_vm_t* vm, SLVAL self)
{
    sl_buffer_t* buff = get_buffer(vm, self);
    ensure_buffer(vm, buff);
    return sl_make_int(vm, buff->len);
}

static SLVAL
buffer_inspect(sl_vm_t* vm, SLVAL self)
{
    sl_buffer_t* buff = get_buffer(vm, self);
    ensure_buffer(vm, buff);
    size_t inspect_len = strlen("#<Buffer:\"");
    size_t inspect_cap = 32;
    char* inspect_str = sl_alloc_buffer(vm->arena, inspect_cap);
    strcpy(inspect_str, "#<Buffer:\"");
    for(size_t i = 0; i < buff->len; i++) {
        if(inspect_len + 16 >= inspect_cap) {
            inspect_cap *= 2;
            inspect_str = sl_realloc(vm->arena, inspect_str, inspect_cap);
        }
        if(buff->buffer[i] >= 32 && buff->buffer[i] <= 127) {
            inspect_str[inspect_len++] = buff->buffer[i];
        } else {
            char hex[8];
            sprintf(hex, "\\x%02x", (uint8_t)buff->buffer[i]);
            inspect_str[inspect_len++] = hex[0];
            inspect_str[inspect_len++] = hex[1];
            inspect_str[inspect_len++] = hex[2];
            inspect_str[inspect_len++] = hex[3];
        }
    }
    inspect_str[inspect_len++] = '"';
    inspect_str[inspect_len++] = '>';
    return sl_make_string_no_copy(vm, (uint8_t*)inspect_str, inspect_len);
}

static SLVAL
buffer_decode(sl_vm_t* vm, SLVAL self, SLVAL from_encoding)
{
    sl_buffer_t* buff = get_buffer(vm, self);
    char* encoding = sl_to_cstr(vm, from_encoding);
    size_t out_len;
    char* out_buff = sl_iconv(vm, buff->buffer, buff->len, encoding, "UTF-8", &out_len);
    return sl_make_string_no_copy(vm, (uint8_t*)out_buff, out_len);
}

sl_frozen_buffer_t*
sl_buffer_get_frozen_copy(sl_vm_t* vm, SLVAL buffer)
{
    sl_buffer_t* buff = get_buffer(vm, buffer);
    sl_frozen_buffer_t* copy = sl_alloc(vm->arena, sizeof(sl_frozen_buffer_t));
    copy->data = sl_alloc_buffer(vm->arena, buff->len);
    memcpy(copy->data, buff->buffer, buff->len);
    copy->len = buff->len;
    return copy;
}

void
sl_init_buffer(sl_vm_t* vm)
{
    vm->lib.Buffer = sl_define_class(vm, "Buffer", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.Buffer, allocate_buffer);
    sl_define_method(vm, vm->lib.Buffer, "init", -1, buffer_init);
    sl_define_method(vm, vm->lib.Buffer, "[]", 1, buffer_get);
    sl_define_method(vm, vm->lib.Buffer, "[]=", 2, buffer_set);
    sl_define_method(vm, vm->lib.Buffer, "length", 0, buffer_length);
    sl_define_method(vm, vm->lib.Buffer, "inspect", 0, buffer_inspect);
    sl_define_method(vm, vm->lib.Buffer, "decode", 1, buffer_decode);
}
