#include <string.h>
#include <iconv.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <slash/string.h>
#include <slash/value.h>
#include <slash/vm.h>
#include <slash/class.h>
#include <slash/utf8.h>
#include <slash/unicode.h>
#include <slash/lib/array.h>
#include <slash/lib/number.h>
#include <slash/lib/regexp.h>
#include <slash/lib/buffer.h>
#include <slash/lib/enumerable.h>
#include <slash/lib/range.h>

static int
str_hash(sl_vm_t* vm, sl_string_t* str)
{
    uint32_t m;
    uint32_t r;
    uint32_t seed;

    int hash;
    int k;
    size_t i;

    if(str->base.user_flags & SL_FLAG_STRING_HASH_SET) {
        return str->hash;
    }

    m = 0x5bd1e995;
    r = 24;
    seed = 0x9747b28c ^ vm->hash_seed;
    hash = seed ^ str->buff_len;

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

    str->base.user_flags |= SL_FLAG_STRING_HASH_SET;
    return str->hash = hash;
    (void)vm;
}

static int
str_cmp(sl_vm_t* vm, sl_string_t* a, sl_string_t* b)
{
    for(size_t i = 0;; i++) {
        if(i == a->buff_len && i == b->buff_len) {
            return 0;
        }
        if(i == a->buff_len) {
            return -1;
        }
        if(i == b->buff_len) {
            return 1;
        }
        if(a->buff[i] != b->buff[i]) {
            return a->buff[i] - b->buff[i];
        }
    }
    return 0;
    (void)vm;
}

char*
sl_iconv(sl_vm_t* vm, char* input_string, size_t input_length, char* from_encoding, char* to_encoding, size_t* output_length)
{
    iconv_t cd = iconv_open(to_encoding, from_encoding);
    if(cd == (iconv_t)(-1)) {
        sl_throw_message2(vm, vm->lib.EncodingError, "Unknown encoding");
    }
    char* in_buff = input_string;
    size_t in_bytes_left = input_length;
    size_t out_bytes_left = in_bytes_left * 4 + 15;
    size_t out_cap = out_bytes_left;
    char* out_buff = sl_alloc_buffer(vm->arena, out_cap);
    char* retn_out_buff = out_buff;
    while(1) {
        size_t ret = iconv(cd, &in_buff, &in_bytes_left, &out_buff, &out_bytes_left);
        if(ret != (size_t)(-1)) {
            break;
        }
        if(errno == E2BIG) {
            out_bytes_left = input_length;
            out_cap += input_length;
            out_buff = sl_realloc(vm->arena, out_buff, out_cap);
            continue;
        }
        if(errno == EILSEQ || errno == EINVAL) {
            iconv_close(cd);
            sl_throw_message2(vm, vm->lib.EncodingError, "Invalid encoding in source buffer");
        }
        break;
    }
    iconv_close(cd);
    *output_length = out_cap - out_bytes_left;
    return retn_out_buff;
}

struct sl_st_hash_type
sl_string_hash_type = { str_cmp, str_hash };

static void
ensure_utf8(sl_vm_t* vm, uint8_t* buff, size_t buff_len)
{
    if(!sl_is_valid_utf8(buff, buff_len)) {
        sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8");
    }
}

SLVAL
sl_make_string_no_copy(sl_vm_t* vm, uint8_t* buff, size_t buff_len)
{
    ensure_utf8(vm, buff, buff_len);

    SLVAL vstr = sl_allocate(vm, vm->lib.String);
    sl_string_t* str = (sl_string_t*)sl_get_ptr(vstr);
    str->char_len = sl_utf8_strlen(vm, buff, buff_len);
    str->buff = buff;
    str->buff_len = buff_len;
    return vstr;
}

SLVAL
sl_make_string(struct sl_vm* vm, uint8_t* buff, size_t buff_len)
{
    uint8_t* our_buff = sl_alloc_buffer(vm->arena, buff_len + 1);
    memcpy(our_buff, buff, buff_len);
    return sl_make_string_no_copy(vm, our_buff, buff_len);
}

SLVAL
sl_make_cstring(struct sl_vm* vm, const char* cstr)
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

sl_string_t*
sl_get_string(sl_vm_t* vm, SLVAL obj)
{
    sl_expect(vm, obj, vm->lib.String);
    return (sl_string_t*)sl_get_ptr(obj);
}

SLVAL
sl_string_length(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, sl_get_string(vm, self)->char_len);
}

static SLVAL
sl_string_byte_length(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, sl_get_string(vm, self)->buff_len);
}

SLVAL
sl_string_concat(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    sl_string_t* a = sl_get_string(vm, self);
    sl_string_t* b = sl_get_string(vm, other);
    uint8_t* buff = (uint8_t*)sl_alloc_buffer(vm->arena, a->buff_len + b->buff_len);
    memcpy(buff, a->buff, a->buff_len);
    memcpy(buff + a->buff_len, b->buff, b->buff_len);
    return sl_make_string(vm, buff, a->buff_len + b->buff_len);
}

SLVAL
sl_string_times(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    sl_string_t* str = sl_get_string(vm, self);
    long mul = sl_get_int(sl_expect(vm, other, vm->lib.Int));
    if(mul == 0) {
        return sl_make_cstring(vm, "");
    }
    if(mul < 0) {
        sl_throw_message2(vm, vm->lib.ArgumentError, "String multiplier must be positive");
    }
    if(mul > 0 && (size_t)LONG_MAX / mul < str->buff_len) {
        sl_throw_message2(vm, vm->lib.ArgumentError, "String multiplier is too big");
    }
    sl_string_t* new_str = sl_get_string(vm, sl_make_string(vm, NULL, 0));
    new_str->buff_len = str->buff_len * mul;
    new_str->buff = sl_alloc(vm->arena, new_str->buff_len);
    for(size_t i = 0; i < new_str->buff_len; i += str->buff_len) {
        memcpy(new_str->buff + i, str->buff, str->buff_len);
    }
    new_str->char_len = str->char_len * mul;
    return sl_make_ptr((sl_object_t*)new_str);
}

SLVAL
sl_string_html_escape(sl_vm_t* vm, SLVAL self)
{
    sl_string_t* str = sl_get_string(vm, self);
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
    sl_string_t* str = sl_get_string(vm, self);
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
    sl_string_t* str = sl_get_string(vm, self);
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
        for(unsigned int i = 0; i < utf8len; i++) {
            sprintf((char*)out + out_len, "%%%2X", utf8buff[i]);
            out_len += 3;
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
    sl_string_t* str = sl_get_string(vm, self);
    return sl_integer_parse(vm, str->buff, str->buff_len);
}

static SLVAL
sl_string_to_f(sl_vm_t* vm, SLVAL self)
{
    char* str = sl_to_cstr(vm, self);
    double dbl = 0;
    sscanf(str, "%lf", &dbl);
    return sl_make_float(vm, dbl);
}

SLVAL
sl_string_inspect(sl_vm_t* vm, SLVAL self)
{
    sl_string_t* str = sl_get_string(vm, self);
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
        } else if(str->buff[str_i] == '\\') {
            memcpy(out + out_len, "\\\\", 2);
            out_len += 2;
        } else if(str->buff[str_i] == '\n') {
            memcpy(out + out_len, "\\n", 2);
            out_len += 2;
        } else if(str->buff[str_i] == '\r') {
            memcpy(out + out_len, "\\r", 2);
            out_len += 2;
        } else if(str->buff[str_i] == '\t') {
            memcpy(out + out_len, "\\t", 2);
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

SLVAL
sl_string_eq(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(!sl_is_a(vm, other, vm->lib.String)) {
        return vm->lib._false;
    }
    sl_string_t* a = sl_get_string(vm, self);
    sl_string_t* b = sl_get_string(vm, other);
    return sl_make_bool(vm, str_cmp(vm, a, b) == 0);
}

SLVAL
sl_string_is_match(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(sl_is_a(vm, other, vm->lib.String)) {
        SLVAL idx = sl_string_index(vm, self, other);
        return sl_make_bool(vm, sl_get_ptr(idx) != sl_get_ptr(vm->lib.nil));
    }
    return sl_regexp_is_match(vm, other, self);
}

SLVAL
sl_string_index(sl_vm_t* vm, SLVAL self, SLVAL substr)
{
    sl_string_t* haystack = sl_get_string(vm, self);
    sl_string_t* needle = sl_get_string(vm, substr);

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

static SLVAL
string_range_index(sl_vm_t* vm, SLVAL self, SLVAL range)
{
    sl_string_t* str = sl_get_string(vm, self);
    SLVAL lowerv = sl_range_lower(vm, range);
    SLVAL upperv = sl_range_upper(vm, range);
    if(!sl_is_a(vm, lowerv, vm->lib.Int) || !sl_is_a(vm, upperv, vm->lib.Int)) {
        sl_throw_message2(vm, vm->lib.TypeError, "Expected range of integers");
    }
    long lower = sl_get_int(lowerv), upper = sl_get_int(upperv);
    if(lower < 0) {
        lower += str->char_len;
    }
    if(lower < 0 || (size_t)lower >= str->char_len) {
        return sl_make_cstring(vm, "");
    }
    if(upper < 0) {
        upper += str->char_len;
    }
    if(upper < 0) {
        return sl_make_cstring(vm, "");
    }
    if(sl_range_is_exclusive(vm, range)) {
        upper--;
    }
    if(upper < lower) {
        return sl_make_cstring(vm, "");
    }
    uint8_t* begin_ptr = str->buff;
    uint8_t* end_ptr;
    size_t len = str->buff_len;
    long idx = 0;
    while(idx < lower && len) {
        idx++;
        sl_utf8_each_char(vm, &begin_ptr, &len);
    }
    end_ptr = begin_ptr;
    while(lower <= upper) {
        lower++;
        sl_utf8_each_char(vm, &end_ptr, &len);
    }
    return sl_make_string(vm, begin_ptr, (size_t)end_ptr - (size_t)begin_ptr);
}

SLVAL
sl_string_char_at_index(sl_vm_t* vm, SLVAL self, SLVAL index)
{
    sl_string_t* str = sl_get_string(vm, self);
    if(sl_is_a(vm, index, vm->lib.Range_Inclusive) || sl_is_a(vm, index, vm->lib.Range_Exclusive)) {
        return string_range_index(vm, self, index);
    }
    long idx = sl_get_int(sl_expect(vm, index, vm->lib.Int));
    if(idx < 0) {
        idx += str->char_len;
    }
    if(idx < 0 || idx >= (long)str->char_len) {
        return vm->lib.nil;
    }
    uint8_t* buff_ptr = str->buff;
    size_t len = str->buff_len;
    while(idx) {
        sl_utf8_each_char(vm, &buff_ptr, &len);
        idx--;
    }
    size_t slice_len = 1;
    while(slice_len < len && (buff_ptr[slice_len] & 0xc0) == 0x80) {
        slice_len++;
    }
    return sl_make_string(vm, buff_ptr, slice_len);
}

SLVAL
sl_string_split(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    SLVAL substr;
    sl_string_t* haystack = sl_get_string(vm, self);
    sl_string_t* needle;
    SLVAL ret = sl_make_array(vm, 0, NULL), piece;
    uint8_t* haystack_buff = haystack->buff;
    uint8_t* start_ptr = haystack_buff;
    size_t haystack_len = haystack->buff_len;
    uint8_t buff[12];
    size_t buff_len;
    uint32_t c;
    long limit = 0;

    if(argc > 0) {
        substr = argv[0];
    } else {
        substr = sl_make_cstring(vm, " ");
    }

    needle = sl_get_string(vm, substr);

    if(argc > 1) {
        limit = sl_get_int(sl_expect(vm, argv[1], vm->lib.Int));
        if(limit < 0) {
            limit = 0;
        }
    }
    long length = 0;
    if(needle->buff_len == 0) {
        while(haystack_len) {
            length++;
            if(limit && length == limit) {
                SLVAL rest = sl_make_string(vm, haystack_buff, haystack_len);
                sl_array_push(vm, ret, 1, &rest);
                break;
            }
            c = sl_utf8_each_char(vm, &haystack_buff, &haystack_len);
            buff_len = sl_utf32_char_to_utf8(vm, c, buff);
            piece = sl_make_string(vm, buff, buff_len);
            sl_array_push(vm, ret, 1, &piece);
        }
        return ret;
    } else {
        if(limit == 1) {
            return sl_make_array(vm, 1, &self);
        }
        while(haystack_len >= needle->buff_len) {
            if(memcmp(haystack_buff, needle->buff, needle->buff_len) == 0) {
                piece = sl_make_string(vm, start_ptr, haystack_buff - start_ptr);
                sl_array_push(vm, ret, 1, &piece);
                haystack_buff += needle->buff_len;
                haystack_len -= needle->buff_len;
                length++;
                if(limit && length + 1 == limit) {
                    SLVAL rest = sl_make_string(vm, haystack_buff, haystack_len);
                    sl_array_push(vm, ret, 1, &rest);
                    return ret;
                }
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

SLVAL
sl_string_encode(sl_vm_t* vm, SLVAL self, char* encoding)
{
    sl_string_t* str = sl_get_string(vm, self);
    size_t out_len;
    char* out_buff = sl_iconv(vm, (char*)str->buff, str->buff_len, "UTF-8", encoding, &out_len);
    return sl_make_buffer(vm, out_buff, out_len);
}

SLVAL
sl_string_encode2(sl_vm_t* vm, SLVAL self, SLVAL encoding)
{
    return sl_string_encode(vm, self, sl_to_cstr(vm, encoding));
}

int
sl_string_byte_offset_for_index(sl_vm_t* vm, SLVAL strv, int index)
{
    sl_string_t* str = sl_get_string(vm, strv);
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

int
sl_string_index_for_byte_offset(sl_vm_t* vm, SLVAL strv, int byte_offset)
{
    sl_string_t* str = sl_get_string(vm, strv);
    uint8_t* buff = str->buff;
    size_t len = str->buff_len;
    int index = 0;
    while(len && byte_offset > 0) {
        size_t old_len = len;
        sl_utf8_each_char(vm, &buff, &len);
        index++;
        byte_offset -= old_len - len;
    }
    if(byte_offset > 0) {
        return -1;
    }
    return index;
}

static SLVAL
sl_string_spaceship(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    return sl_make_int(vm, str_cmp(vm, sl_get_string(vm, self), sl_get_string(vm, other)));
}

static SLVAL
sl_string_hash(sl_vm_t* vm, SLVAL self)
{
    return sl_make_int(vm, str_hash(vm, sl_get_string(vm, self)));
}

void
sl_pre_init_string(sl_vm_t* vm)
{
    SLID no_id = { 0 };
    vm->lib.String = sl_define_class2(vm, no_id, vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.String, allocate_string);
}

static SLVAL
sl_string_replace(sl_vm_t* vm, SLVAL self, SLVAL search, SLVAL replace)
{
    if(sl_is_a(vm, search, vm->lib.String)) {
        return sl_enumerable_join(vm, sl_string_split(vm, self, 1, &search), 1, &replace);
    }

    sl_expect(vm, search, vm->lib.Regexp);

    SLVAL retn = sl_make_cstring(vm, "");

    while(1) {
        SLVAL match = sl_regexp_match(vm, search, 1, &self);

        if(!sl_is_truthy(match)) {
            return sl_string_concat(vm, retn, self);
        } else {
            SLVAL part = sl_regexp_match_before(vm, match);
            if(sl_is_a(vm, replace, vm->lib.String)) {
                part = sl_string_concat(vm, part, replace);
            } else {
                part = sl_string_concat(vm, part, sl_send_id(vm, replace, vm->id.call, 1, match));
            }
            retn = sl_string_concat(vm, retn, part);
        }

        self = sl_regexp_match_after(vm, match);
    }
}

SLVAL
sl_string_upper(sl_vm_t* vm, SLVAL selfv)
{
    sl_string_t* self = sl_get_string(vm, selfv);
    sl_string_t* retn = sl_get_string(vm, sl_allocate(vm, vm->lib.String));
    memcpy(retn, self, sizeof(sl_string_t));
    retn->buff = sl_alloc_buffer(vm->arena, retn->buff_len);

    size_t len = self->buff_len;
    uint8_t* buff = self->buff;
    size_t out_offset = 0;
    uint32_t upper_c;

    while(len) {
        uint32_t c = sl_utf8_each_char(vm, &buff, &len);
        upper_c = sl_unicode_toupper(c);
        out_offset += sl_utf32_char_to_utf8(vm, upper_c, retn->buff + out_offset);
    }

    retn->buff_len = out_offset;

    return sl_make_ptr((sl_object_t*)retn);
}

SLVAL
sl_string_lower(sl_vm_t* vm, SLVAL selfv)
{
    sl_string_t* self = sl_get_string(vm, selfv);
    sl_string_t* retn = sl_get_string(vm, sl_allocate(vm, vm->lib.String));
    memcpy(retn, self, sizeof(sl_string_t));
    retn->buff = sl_alloc_buffer(vm->arena, retn->buff_len);

    size_t len = self->buff_len;
    uint8_t* buff = self->buff;
    size_t out_offset = 0;
    uint32_t lower_c;

    while(len) {
        uint32_t c = sl_utf8_each_char(vm, &buff, &len);
        lower_c = sl_unicode_tolower(c);
        out_offset += sl_utf32_char_to_utf8(vm, lower_c, retn->buff + out_offset);
    }

    retn->buff_len = out_offset;

    return sl_make_ptr((sl_object_t*)retn);
}

void
sl_init_string(sl_vm_t* vm)
{
    sl_st_insert(((sl_class_t*)sl_get_ptr(vm->lib.Object))->constants,
        (sl_st_data_t)sl_intern(vm, "String").id, (sl_st_data_t)vm->lib.String.i);
    sl_define_method(vm, vm->lib.String, "length", 0, sl_string_length);
    sl_define_method(vm, vm->lib.String, "byte_length", 0, sl_string_byte_length);
    sl_define_method(vm, vm->lib.String, "concat", 1, sl_string_concat);
    sl_define_method(vm, vm->lib.String, "+", 1, sl_string_concat);
    sl_define_method(vm, vm->lib.String, "*", 1, sl_string_times);
    sl_define_method(vm, vm->lib.String, "to_s", 0, sl_string_to_s);
    sl_define_method(vm, vm->lib.String, "to_i", 0, sl_string_to_i);
    sl_define_method(vm, vm->lib.String, "to_f", 0, sl_string_to_f);
    sl_define_method(vm, vm->lib.String, "inspect", 0, sl_string_inspect);
    sl_define_method(vm, vm->lib.String, "html_escape", 0, sl_string_html_escape);
    sl_define_method(vm, vm->lib.String, "url_decode", 0, sl_string_url_decode);
    sl_define_method(vm, vm->lib.String, "url_encode", 0, sl_string_url_encode);
    sl_define_method(vm, vm->lib.String, "index", 1, sl_string_index);
    sl_define_method(vm, vm->lib.String, "[]", 1, sl_string_char_at_index);
    sl_define_method(vm, vm->lib.String, "split", -1, sl_string_split);
    sl_define_method(vm, vm->lib.String, "==", 1, sl_string_eq);
    sl_define_method(vm, vm->lib.String, "~", 1, sl_string_is_match);
    sl_define_method(vm, vm->lib.String, "<=>", 1, sl_string_spaceship);
    sl_define_method(vm, vm->lib.String, "hash", 0, sl_string_hash);
    sl_define_method(vm, vm->lib.String, "encode", 1, sl_string_encode2);
    sl_define_method(vm, vm->lib.String, "replace", 2, sl_string_replace);
    sl_define_method(vm, vm->lib.String, "upper", 0, sl_string_upper);
    sl_define_method(vm, vm->lib.String, "lower", 0, sl_string_lower);
}

// somewhat similar to sprintf
//
// Valid formats:
//
//   %V - Slash value, converted to string first with #to_s
//   %I - Slash ID
//   %s - C string
//   %d - integer
//   %% - literal '%' sign
//
// You may add the 'Q' modifier to a format specifier to quote it in double
// quotes. For example "%QV" would quote the value when interpolating it into
// the string
SLVAL
sl_make_formatted_string_va(struct sl_vm* vm, const char* format, va_list va)
{
    SLVAL strings = sl_make_array(vm, 0, NULL);
    const char *current_ptr = format, *next_ptr;
    SLVAL quote = vm->lib.nil;
    while((next_ptr = strchr(current_ptr, '%'))) {
        if(next_ptr != current_ptr) {
            SLVAL literal = sl_make_string(vm, (uint8_t*)current_ptr, (size_t)(next_ptr - current_ptr));
            sl_array_push(vm, strings, 1, &literal);
        }
        SLVAL element = vm->lib.nil;
        bool quoted = false;
    again:
        switch(next_ptr[1]) {
            case 'Q': {
                if(sl_get_primitive_type(quote) == SL_T_NIL) {
                    quote = sl_make_cstring(vm, "\"");
                }
                sl_array_push(vm, strings, 1, &quote);
                next_ptr++;
                quoted = true;
                goto again;
            }
            case 'V': {
                element = va_arg(va, SLVAL);
                break;
            }
            case 'X': {
                element = va_arg(va, SLVAL);
                element = sl_inspect(vm, element);
                break;
            }
            case 'I': {
                SLID id = va_arg(va, SLID);
                element = sl_id_to_string(vm, id);
                break;
            }
            case 's': {
                char* cstr = va_arg(va, char*);
                element = sl_make_cstring(vm, cstr);
                break;
            }
            case 'd': {
                char buff[32];
                int num = va_arg(va, int);
                sprintf(buff, "%d", num);
                element = sl_make_cstring(vm, buff);
                break;
            }
            case 'p': {
                char buff[32];
                void* ptr = va_arg(va, void*);
                sprintf(buff, "%p", ptr);
                element = sl_make_cstring(vm, buff);
                break;
            }
            case 0: {
                return vm->lib.nil;
            }
            default: {
                element = sl_make_string(vm, (uint8_t*)(next_ptr + 1), 1);
                break;
            }
        }
        sl_array_push(vm, strings, 1, &element);

        if(quoted) {
            sl_array_push(vm, strings, 1, &quote);
            quoted = false;
        }

        current_ptr = next_ptr + 2;
    }
    if(current_ptr[0]) {
        SLVAL element = sl_make_cstring(vm, current_ptr);
        sl_array_push(vm, strings, 1, &element);
    }

    return sl_array_join(vm, strings, 0, NULL);
}

SLVAL
sl_make_formatted_string(struct sl_vm* vm, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    SLVAL retn = sl_make_formatted_string_va(vm, format, va);
    va_end(va);
    return retn;
}
