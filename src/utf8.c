#include <slash/utf8.h>
#include <slash/string.h>

static uint32_t utf8_code_point_limit[] = {
    0,
    0x000080,
    0x000800,
    0x010000,
    0x200000
};

size_t
sl_utf8_strlen(sl_vm_t* vm, uint8_t* buff, size_t len)
{
    size_t i, bytes = 0, j, utf8len = 0;
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
                sl_throw(vm, sl_make_error2(vm, vm->lib.EncodingError, sl_make_cstring(vm, "Invalid UTF-8 sequence")));
                return 0; /* never reached */
            }
        } else {
            c32 = c;
            bytes = 1;
        }
        if(i + bytes > len) {
            sl_throw(vm, sl_make_error2(vm, vm->lib.EncodingError, sl_make_cstring(vm, "Invalid UTF-8 sequence")));
        }
        for(j = 1; j < bytes; j++) {
            c32 <<= 6;
            c32 |= buff[++i] & 0x3f;
        }
        if(c32 < utf8_code_point_limit[bytes - 1]) {
            sl_throw(vm, sl_make_error2(vm, vm->lib.EncodingError, sl_make_cstring(vm, "Invalid UTF-8 sequence")));
        }
        utf8len++;
    }
    return utf8len;
}

int
sl_is_valid_utf8(uint8_t* buff, size_t len)
{
    size_t i, bytes = 0, j, utf8len = 0;
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
                return 2;
            }
        } else {
            c32 = c;
            bytes = 1;
        }
        if(i + bytes > len) {
            return 1;
        }
        for(j = 1; j < bytes; j++) {
            c32 <<= 6;
            c32 |= buff[++i] & 0x3f;
        }
        if(c32 < utf8_code_point_limit[bytes - 1]) {
            return 1;
        }
        utf8len++;
    }
    return 1;
}

uint32_t*
sl_utf8_to_utf32(sl_vm_t* vm, uint8_t* buff, size_t len, size_t* out_len)
{
    uint32_t* out = sl_alloc(vm->arena, sizeof(uint32_t) * len);
    size_t str_len = 0;
    size_t clen = len;
    uint8_t* cbuff = buff;
    while(clen) {
        out[str_len++] = sl_utf8_each_char(vm, &cbuff, &clen);
    }
    *out_len = str_len;
    return out;
}

uint32_t
sl_utf8_each_char(sl_vm_t* vm, uint8_t** buff_ptr, size_t* len)
{
    uint32_t c;
    uint8_t* buff = *buff_ptr;
    if(*len == 0) {
        return 0;
    }
    if((buff[0] & 0x80) == 0x00) { /* 1 byte char */
        ++*buff_ptr;
        --*len;
        return buff[0];
    }
    if((buff[0] & 0xe0) == 0xc0) { /* 2 byte char */
        if(*len < 2) sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 sequence");
        if((buff[1] & 0xc0) != 0x80) sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 sequence");
        c  = (buff[0] & 0x1f) << 6;
        c |= (buff[1] & 0x3f) << 0;
        if(c < 128) sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 sequence");
        *buff_ptr += 2;
        *len -= 2;
        return c;
    }
    if((buff[0] & 0xf0) == 0xe0) { /* 3 byte char */
        if(*len < 3) sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 sequence");
        if((buff[1] & 0xc0) != 0x80) sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 sequence");
        if((buff[2] & 0xc0) != 0x80) sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 sequence");
        c  = (buff[0] & 0x0f) << 12;
        c |= (buff[1] & 0x3f) << 6;
        c |= (buff[2] & 0x3f) << 0;
        if(c < 2048) sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 sequence");
        *buff_ptr += 3;
        *len -= 3;
        return c;
    }
    if((buff[0] & 0xf8) == 0xf0) { /* 4 byte char */
        if(*len < 4) sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 sequence");
        if((buff[1] & 0xc0) != 0x80) sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 sequence");
        if((buff[2] & 0xc0) != 0x80) sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 sequence");
        if((buff[3] & 0xc0) != 0x80) sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 sequence");
        c  = (buff[0] & 0x07) << 18;
        c |= (buff[1] & 0x3f) << 12;
        c |= (buff[2] & 0x3f) << 6;
        c |= (buff[3] & 0x3f) << 0;
        if(c < 65536) sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 sequence");
        *buff_ptr += 4;
        *len -= 4;
        return c;
    }
    sl_throw_message2(vm, vm->lib.EncodingError, "Invalid UTF-8 sequence");
    return 0; /* never reached */
}

size_t
sl_utf32_char_to_utf8(sl_vm_t* vm, uint32_t u32c, uint8_t* u8buff)
{
    if(u32c < 0x80) {
        u8buff[0] = (uint8_t)u32c;
        return 1;
    }
    if(u32c < 0x800) {
        u8buff[0] = 0xc0 | ((u32c >>  6) & 0x1f);
        u8buff[1] = 0x80 | ((u32c >>  0) & 0x3f);
        return 2;
    }
    if(u32c < 0x10000) {
        u8buff[0] = 0xe0 | ((u32c >> 12) & 0x0f);
        u8buff[1] = 0x80 | ((u32c >>  6) & 0x3f);
        u8buff[2] = 0x80 | ((u32c >>  0) & 0x3f);
        return 3;
    }
    if(u32c < 0x200000) {
        u8buff[0] = 0xe0 | ((u32c >> 18) & 0x0f);
        u8buff[1] = 0x80 | ((u32c >> 12) & 0x3f);
        u8buff[2] = 0x80 | ((u32c >>  6) & 0x3f);
        u8buff[3] = 0x80 | ((u32c >>  0) & 0x3f);
        return 4;
    }
    sl_throw_message2(vm, vm->lib.EncodingError, "UTF-32 character out of UTF-8 range");
    return 0;
}

uint8_t*
sl_utf32_to_utf8(sl_vm_t* vm, uint32_t* u32, size_t u32_len, size_t* buff_len)
{
    size_t cap = 8;
    size_t len = 0;
    uint8_t* buff = sl_alloc(vm->arena, cap);
    size_t i;
    for(i = 0; i < u32_len; i++) {
        if(cap + 5 >= len) {
            buff = sl_realloc(vm->arena, buff, cap *= 2);
        }
        len += sl_utf32_char_to_utf8(vm, u32[i], buff + len);
    }
    buff[len] = 0;
    *buff_len = len;
    return buff;
}
