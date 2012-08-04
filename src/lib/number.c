#include "class.h"
#include "value.h"
#include "lib/bignum.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#include <gc.h>
#include <limits.h>

SLVAL
sl_integer_parse(sl_vm_t* vm, uint8_t* str, size_t len)
{
    /* TODO handle bignum upgrade as well */
    int n;
    mpz_t mpz;
    char* buff = sl_alloc(vm->arena, len + 1);
    memcpy(buff, str, len);
    buff[len] = 0;
    mpz_init_set_str(mpz, buff, 10);
    if(mpz_cmp_si(mpz, INT_MIN / 2) > 0 && mpz_cmp_si(mpz, INT_MAX / 2) < 0) {
        mpz_clear(mpz);
        sscanf(buff, "%d", &n);
        return sl_make_int(vm, n);
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
