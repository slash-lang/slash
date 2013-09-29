#include <time.h>
#include <slash.h>

typedef struct sl_mt_state {
    int mt[624];
    int index;
}
sl_mt_state_t;

void
generate_numbers(sl_mt_state_t* state)
{
    int i, y;
    for(i = 0; i < 624; i++) {
        y = (state->mt[i] >> 31) + (state->mt[(i + 1) % 624] & 0x7fffffff);
        state->mt[i] = state->mt[(i + 397) % 624] ^ (y >> 1);
        if(y % 2) {
            state->mt[i] ^= 2567483615;
        }
    }
}

int
sl_rand(sl_vm_t* vm)
{
    if(!(vm->inited & SL_INIT_RAND)) {
        sl_rand_init_mt(vm);
        vm->inited |= SL_INIT_RAND;
    }
    if(vm->mt_state->index == 0) {
        generate_numbers(vm->mt_state);
    }
    int y = vm->mt_state->mt[vm->mt_state->index];
    vm->mt_state->index = (vm->mt_state->index + 1) % 624;
    y ^= y >> 11;
    y ^= (y << 7) & 2636928640;
    y ^= (y << 15) & 4022730752;
    y ^= y >> 18;
    return y;
}

void
sl_rand_init_mt(sl_vm_t* vm)
{
    int seed = sl_seed(), i;
    vm->mt_state = sl_alloc_buffer(vm->arena, sizeof(*vm->mt_state));
    vm->mt_state->mt[0] = seed;
    for(i = 1; i < 624; i++) {
        vm->mt_state->mt[i] = 1812433253 * (vm->mt_state->mt[i - 1] ^ (vm->mt_state->mt[i - 1] >> 30)) + 1;
    }
}

static SLVAL
sl_rand_v(sl_vm_t* vm)
{
    return sl_make_float(vm, sl_rand(vm) / (double)INT_MAX);
}

void
sl_init_rand(sl_vm_t* vm)
{
    sl_define_method(vm, vm->lib.Object, "rand", 0, sl_rand_v);
}
