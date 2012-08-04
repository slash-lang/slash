#include <string.h>
#include <iconv.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "value.h"
#include "vm.h"
#include "class.h"
#include "utf8.h"
#include "lib/array.h"
#include "lib/number.h"

static int
str_hash(sl_string_t* str)
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
str_cmp(sl_string_t* a, sl_string_t* b)
{
    if(a->buff_len < b->buff_len) {
        return -1;
    } else if(a->buff_len > b->buff_len) {
        return 1;
    }
    return memcmp(a->buff, b->buff, a->buff_len);
}

struct st_hash_type
sl_string_hash_type = { str_cmp, str_hash };

SLVAL
sl_make_string(sl_vm_t* vm, uint8_t* buff, size_t buff_len)
{
    SLVAL vstr = sl_allocate(vm, vm->lib.String);
    sl_string_t* str = (sl_string_t*)sl_get_ptr(vstr);
    str->char_len = sl_utf8_strlen(vm, buff, buff_len);
    str->buff = sl_alloc_buffer(vm->arena, buff_len + 1);
    memcpy(str->buff, buff, buff_len);
    str->buff[buff_len] = 0;
    str->buff_len = buff_len;
    return vstr;
}

SLVAL
sl_make_string_enc(sl_vm_t* vm, char* buff, size_t buff_len, char* encoding)
{
    size_t in_bytes_left = buff_len, out_bytes_left = buff_len * 4 + 15, cap = out_bytes_left;
    char *inbuff = buff, *outbuf = sl_alloc_buffer(vm->arena, out_bytes_left), *retn_outbuf = outbuf;
    size_t ret;
    iconv_t cd = iconv_open("UTF-8", encoding);
    if(cd == (iconv_t)(-1)) {
        sl_throw_message2(vm, vm->lib.EncodingError, "Unknown source encoding");
    }
    while(1) {
        ret = iconv(cd, &inbuff, &in_bytes_left, &outbuf, &out_bytes_left);
        
        if(ret != (size_t)-1) {
            break;
        }
        if(errno == E2BIG) {
            out_bytes_left = buff_len;
            cap += buff_len;
            outbuf = sl_realloc(vm->arena, outbuf, cap);
            continue;
        }
        
        if(errno == EILSEQ || errno == EINVAL) {
            sl_throw_message2(vm, vm->lib.EncodingError, "Invalid source string");
        }
        break;
    }
    iconv_close(cd);
    return sl_make_string(vm, (uint8_t*)retn_outbuf, cap - out_bytes_left);
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
allocate_string(sl_vm_t* vm)
{
    sl_object_t* obj = (sl_object_t*)sl_alloc(vm->arena, sizeof(sl_string_t));
    obj->primitive_type = SL_T_STRING;
    return obj;
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

SLVAL
sl_string_concat(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    sl_string_t* a = get_string(vm, self);
    sl_string_t* b = get_string(vm, other);
    uint8_t* buff = (uint8_t*)sl_alloc_buffer(vm->arena, a->buff_len + b->buff_len);
    memcpy(buff, a->buff, a->buff_len);
    memcpy(buff + a->buff_len, b->buff, b->buff_len);
    return sl_make_string(vm, buff, a->buff_len + b->buff_len);
}

SLVAL
sl_string_html_escape(sl_vm_t* vm, SLVAL self)
{
    sl_string_t* str = get_string(vm, self);
    size_t out_cap = 32;
    size_t out_len = 0;
    size_t str_i;
    uint8_t* out = sl_alloc_buffer(vm->arena, out_cap);
    for(str_i = 0; str_i < str->buff_len; str_i++) {
        if(out_len + 8 >= out_cap) {
            out_cap *= 2;
            out = sl_realloc(vm->arena, out, out_cap);
        }
        if(str->buff[str_i] == '<') {
            memcpy(out + out_len, "&lt;", 4);
            out_len += 4;
        } else if(str->buff[str_i] == '>') {
            memcpy(out + out_len, "&gt;", 4);
            out_len += 4;
        } else if(str->buff[str_i] == '"') {
            memcpy(out + out_len, "&quot;", 6);
            out_len += 6;
        } else if(str->buff[str_i] == '\'') {
            memcpy(out + out_len, "&#039;", 6);
            out_len += 6;
        } else if(str->buff[str_i] == '&') {
            memcpy(out + out_len, "&amp;", 5);
            out_len += 5;
        } else {
            out[out_len++] = str->buff[str_i];
        }
    }
    return sl_make_string(vm, out, out_len);
}

static int
is_hex_char(uint8_t c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

SLVAL
sl_string_url_decode(sl_vm_t* vm, SLVAL self)
{
    sl_string_t* str = get_string(vm, self);
    size_t out_cap = 32;
    size_t out_len = 0;
    uint8_t* out = sl_alloc_buffer(vm->arena, out_cap);
    size_t str_i;
    char tmp[3];
    for(str_i = 0; str_i < str->buff_len; str_i++) {
        if(out_len + 8 >= out_cap) {
            out_cap *= 2;
            out = sl_realloc(vm->arena, out, out_cap);
        }
        if(str->buff[str_i] == '%') {
            if(str_i + 2 < str->buff_len) {
                if(is_hex_char(str->buff[str_i + 1]) && is_hex_char(str->buff[str_i + 2])) {
                    tmp[0] = str->buff[str_i + 1];
                    tmp[1] = str->buff[str_i + 2];
                    tmp[2] = 0;
                    out[out_len++] = strtol(tmp, NULL, 16);
                    str_i += 2;
                    continue;
                }
            }
        }
        if(str->buff[str_i] == '+') {
            out[out_len++] = ' ';
            continue;
        }
        out[out_len++] = str->buff[str_i];
    }
    return sl_make_string(vm, out, out_len);
}

SLVAL
sl_string_url_encode(sl_vm_t* vm, SLVAL self)
{
    sl_string_t* str = get_string(vm, self);
    size_t out_cap = 32;
    size_t out_len = 0;
    uint8_t* out = sl_alloc_buffer(vm->arena, out_cap);
    size_t clen = str->buff_len;
    uint8_t* cbuff = str->buff;
    uint32_t c;
    uint8_t utf8buff[8];
    uint32_t utf8len;
    while(clen) {
        if(out_len + 16 >= out_cap) {
            out_cap *= 2;
            out = sl_realloc(vm->arena, out, out_cap);
        }
        c = sl_utf8_each_char(vm, &cbuff, &clen);
        if(c >= 'A' && c <= 'Z') {
            out[out_len++] = c;
            continue;
        }
        if(c >= 'a' && c <= 'z') {
            out[out_len++] = c;
            continue;
        }
        if(c >= '0' && c <= '9') {
            out[out_len++] = c;
            continue;
        }
        if(c == '-' || c == '_' || c == '.' || c == '~') {
            out[out_len++] = c;
            continue;
        }
        if(c == ' ') {
            out[out_len++] = '+';
            continue;
        }
        utf8len = sl_utf32_char_to_utf8(vm, c, utf8buff);
        while(utf8len) {
            sprintf((char*)out + out_len, "%%%2X", utf8buff[0]);
            out_len += 3;
            utf8len--;
        }
    }
    return sl_make_string(vm, out, out_len);
}

static SLVAL
sl_string_to_s(sl_vm_t* vm, SLVAL self)
{
    (void)vm;
    return self;
}

static SLVAL
sl_string_to_i(sl_vm_t* vm, SLVAL self)
{
    sl_string_t* str = get_string(vm, self);
    return sl_integer_parse(vm, str->buff, str->buff_len);
}

SLVAL
sl_string_inspect(sl_vm_t* vm, SLVAL self)
{
    sl_string_t* str = get_string(vm, self);
    size_t out_cap = 32;
    size_t out_len = 0;
    size_t str_i;
    uint8_t* out = sl_alloc_buffer(vm->arena, out_cap);
    out[out_len++] = '"';
    for(str_i = 0; str_i < str->buff_len; str_i++) {
        if(out_len + 8 >= out_cap) {
            out_cap *= 2;
            out = sl_realloc(vm->arena, out, out_cap);
        }
        if(str->buff[str_i] == '"') {
            memcpy(out + out_len, "\\\"", 2);
            out_len += 2;
        } else if(str->buff[str_i] < 0x20) {
            out[out_len++] = '\\';
            out[out_len++] = 'x';
            out[out_len++] = '0' + str->buff[str_i] / 0x10;
            if(str->buff[str_i] % 0x10 < 10) {
                out[out_len++] = '0' + str->buff[str_i] % 0x10;
            } else {
                out[out_len++] = 'A' + (str->buff[str_i] % 0x10) - 10;
            }
        } else {
            out[out_len++] = str->buff[str_i];
        }
    }
    out[out_len++] = '"';
    return sl_make_string(vm, out, out_len);
}

static SLVAL
sl_string_eq(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(!sl_is_a(vm, other, vm->lib.String)) {
        return vm->lib._false;
    }
    if(str_cmp((sl_string_t*)sl_get_ptr(self), (sl_string_t*)sl_get_ptr(other)) == 0) {
        return vm->lib._true;
    } else {
        return vm->lib._false;
    }
}

SLVAL
sl_string_index(sl_vm_t* vm, SLVAL self, SLVAL substr)
{
    sl_string_t* haystack = get_string(vm, self);
    sl_string_t* needle = get_string(vm, substr);
    
    /* @TODO use a more efficient algorithm */
    uint8_t* haystack_buff = haystack->buff;
    size_t haystack_len = haystack->buff_len;
    size_t i = 0;
    while(haystack_len >= needle->buff_len) {
        if(memcmp(haystack_buff, needle->buff, needle->buff_len) == 0) {
            return sl_make_int(vm, i);
        }
        sl_utf8_each_char(vm, &haystack_buff, &haystack_len);
        i++;
    }
    
    return vm->lib.nil;
}

SLVAL
sl_string_split(sl_vm_t* vm, SLVAL self, SLVAL substr)
{
    sl_string_t* haystack = get_string(vm, self);
    sl_string_t* needle = get_string(vm, substr);
    SLVAL ret = sl_make_array(vm, 0, NULL), piece;
    uint8_t* haystack_buff = haystack->buff;
    uint8_t* start_ptr = haystack_buff;
    size_t haystack_len = haystack->buff_len;
    uint8_t buff[12];
    size_t buff_len;
    uint32_t c;
    if(needle->buff_len == 0) {
        while(haystack_len) {
            c = sl_utf8_each_char(vm, &haystack_buff, &haystack_len);
            buff_len = sl_utf32_char_to_utf8(vm, c, buff);
            piece = sl_make_string(vm, buff, buff_len);
            sl_array_push(vm, ret, 1, &piece);
        }
        return ret;
    } else {
        while(haystack_len >= needle->buff_len) {
            if(memcmp(haystack_buff, needle->buff, needle->buff_len) == 0) {
                piece = sl_make_string(vm, start_ptr, haystack_buff - start_ptr);
                sl_array_push(vm, ret, 1, &piece);
                haystack_buff += needle->buff_len;
                haystack_len -= needle->buff_len;
                start_ptr = haystack_buff;
                continue;
            }
            haystack_buff++;
            haystack_len--;
        }    
        piece = sl_make_string(vm, start_ptr, haystack_buff - start_ptr + haystack_len);
        sl_array_push(vm, ret, 1, &piece);
        return ret;
    }
    return vm->lib.nil;
}

int
sl_string_byte_offset_for_index(sl_vm_t* vm, SLVAL strv, int index)
{
    sl_string_t* str = get_string(vm, strv);
    uint8_t* buff = str->buff;
    size_t len = str->buff_len;
    while(len) {
        if(index == 0) {
            return buff - str->buff;
        }
        sl_utf8_each_char(vm, &buff, &len);
        index--;
    }
    return -1;
}

static SLVAL
sl_string_spaceship(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    return sl_make_int(vm, str_cmp(get_string(vm, self), get_string(vm, other)));
}

static SLVAL
sl_string_hash(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, str_hash(get_string(vm, self)));
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
    st_insert(((sl_class_t*)sl_get_ptr(vm->lib.Object))->constants,
        (st_data_t)sl_cstring(vm, "String"), (st_data_t)vm->lib.String.i);
    sl_define_method(vm, vm->lib.String, "length", 0, sl_string_length);
    sl_define_method(vm, vm->lib.String, "concat", 0, sl_string_concat);
    sl_define_method(vm, vm->lib.String, "+", 1, sl_string_concat);
    sl_define_method(vm, vm->lib.String, "to_s", 0, sl_string_to_s);
    sl_define_method(vm, vm->lib.String, "to_i", 0, sl_string_to_i);
    sl_define_method(vm, vm->lib.String, "inspect", 0, sl_string_inspect);
    sl_define_method(vm, vm->lib.String, "html_escape", 0, sl_string_html_escape);
    sl_define_method(vm, vm->lib.String, "url_decode", 0, sl_string_url_decode);
    sl_define_method(vm, vm->lib.String, "url_encode", 0, sl_string_url_encode);
    sl_define_method(vm, vm->lib.String, "index", 1, sl_string_index);
    sl_define_method(vm, vm->lib.String, "split", 1, sl_string_split);
    sl_define_method(vm, vm->lib.String, "==", 1, sl_string_eq);
    sl_define_method(vm, vm->lib.String, "<=>", 1, sl_string_spaceship);
    sl_define_method(vm, vm->lib.String, "hash", 0, sl_string_hash);
}
