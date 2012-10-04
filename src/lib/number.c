#include <slash/class.h>
#include <slash/value.h>
#include <slash/lib/bignum.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#include <limits.h>

SLVAL
sl_integer_parse(sl_vm_t* vm, uint8_t* str, size_t len)
{
    mpz_t mpz;
    char* buff = sl_alloc(vm->arena, len + 1);
    memcpy(buff, str, len);
    buff[len] = 0;
    char* dec = memchr(buff, '.', len);
    if(dec) {
        *dec = 0;
    }
    mpz_init_set_str(mpz, buff, 10);
    if(mpz_cmp_si(mpz, INT_MIN / 2) > 0 && mpz_cmp_si(mpz, INT_MAX / 2) < 0) {
        SLVAL retn = sl_make_int(vm, mpz_get_si(mpz));
        mpz_clear(mpz);
        return retn;
    } else {
        mpz_clear(mpz);
        return sl_make_bignum_s(vm, buff);
    }
}

void
sl_init_number(sl_vm_t* vm)
{
    vm->lib.Number = sl_define_class(vm, "Number", vm->lib.Comparable);
}
