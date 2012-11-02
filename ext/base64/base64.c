#include <string.h>
#include <slash.h>

static const char enc_table[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char dec_table[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

static void
enc_block(uint8_t* in, size_t len, uint8_t* out)
{
    out[0] = enc_table[in[0] >> 2];
    out[1] = enc_table[((in[0] & 3) << 4) | (in[1] >> 4)];
    if(len > 1) {
        out[2] = enc_table[((in[1] & 15) << 2) | (in[2] >> 6)];
        if(len > 2) {
            out[3] = enc_table[in[2] & 0x3f];
        } else {
            out[3] = '=';
        }
    } else {
        out[2] = '=';
        out[3] = '=';
    }
}

static SLVAL
sl_base64_encode(sl_vm_t* vm, SLVAL self, SLVAL strv)
{
    sl_string_t* str = sl_get_string(vm, strv);
    size_t out_len = ((str->buff_len + 2) / 3) * 4;
    uint8_t* out_buff = sl_alloc_buffer(vm->arena, out_len);
    size_t i, len;
    len = str->buff_len;
    for(i = 0; i < str->buff_len; i++) {
        enc_block(str->buff + i * 3, len > 3 ? 3 : len, out_buff + i * 4);
        len -= 3;
    }
    return sl_make_string(vm, out_buff, out_len);
    (void)self;
}

static void
dec_block(uint8_t* in, uint8_t* out)
{
    out[0] = (in[0] << 2) | (in[1] >> 4);
    out[1] = (in[1] << 4) | (in[2] >> 2);
    out[2] = ((in[2] << 6) & 0xc0) | in[3];
}

static SLVAL
sl_base64_decode(sl_vm_t* vm, SLVAL self, SLVAL strv)
{
    sl_string_t* str = sl_get_string(vm, strv);
    uint8_t scratch_buff[4];
    int scratch_i = 0;
    uint8_t* out_buff = sl_alloc_buffer(vm->arena, str->buff_len);
    size_t out_i = 0;
    size_t i;
    for(i = 0; i < str->buff_len; i++) {
        if(str->buff[i] >= 'A' && str->buff[i] <= 'Z') {
            scratch_buff[scratch_i++] = str->buff[i] - 'A' + 0;
        } else if(str->buff[i] >= 'a' && str->buff[i] <= 'z') {
            scratch_buff[scratch_i++] = str->buff[i] - 'a' + 26;
        } else if(str->buff[i] >= '0' && str->buff[i] <= '9') {
            scratch_buff[scratch_i++] = str->buff[i] - '0' + 52;
        } else if(str->buff[i] == '+') {
            scratch_buff[scratch_i++] = 62;
        } else if(str->buff[i] == '/') {
            scratch_buff[scratch_i++] = 63;
        } else {
            continue;
        }
        if(scratch_i == 4) {
            dec_block(scratch_buff, out_buff + out_i);
            out_i += 3;
            scratch_i = 0;
            memset(scratch_buff, 0, sizeof(scratch_buff));
        }
    }
    dec_block(scratch_buff, out_buff + out_i);
    if(str->buff_len >= 1 && str->buff[str->buff_len - 1] == '=') {
        if(str->buff_len >= 2 && str->buff[str->buff_len - 2] == '=') {
            out_i += 1;
        } else {
            out_i += 2;
        }
    }
    return sl_make_string(vm, out_buff, out_i);
    (void)self;
}

void
sl_init_ext_base64(sl_vm_t* vm)
{
    SLVAL Base64 = sl_define_class(vm, "Base64", vm->lib.Object);
    sl_define_singleton_method(vm, Base64, "encode", 1, sl_base64_encode);
    sl_define_singleton_method(vm, Base64, "decode", 1, sl_base64_decode);
}
