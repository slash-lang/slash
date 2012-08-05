#include <time.h>
#include "lib/rand.h"
#include "lib/float.h"
#include "slash.h"
#include "platform.h"

static int mt;

typedef struct {
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
    int y;
    sl_mt_state_t* state = (sl_mt_state_t*)sl_get_ptr(sl_vm_store_get(vm, &mt));
    if(state->index == 0) {
        generate_numbers(state);
    }
    y = state->mt[state->index];
    state->index = (state->index + 1) % 624;
    y ^= y >> 11;
    y ^= (y << 7) & 2636928640;
    y ^= (y << 15) & 4022730752;
    y ^= y >> 18;
    return y;
}

void
sl_rand_init_mt(sl_vm_t* vm)
{
    sl_mt_state_t* state = sl_alloc_buffer(vm->arena, sizeof(sl_mt_state_t));
    int seed = sl_seed(), i;
    state->mt[0] = seed;
    for(i = 1; i < 624; i++) {
        state->mt[i] = 1812433253 * (state->mt[i - 1] ^ (state->mt[i - 1] >> 30)) + 1;
    }
    sl_vm_store_put(vm, &mt, sl_make_ptr((sl_object_t*)state));
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
