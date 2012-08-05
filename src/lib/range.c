#include "lib/range.h"
#include "object.h"
#include "class.h"
#include "string.h"
#include "mem.h"

typedef struct {
    sl_object_t base;
    SLVAL left;
    SLVAL right;
    int exclusive;
}
sl_range_t;

static sl_object_t*
allocate_range(sl_vm_t* vm)
{
    sl_range_t* range = sl_alloc(vm->arena, sizeof(sl_range_t));
    range->left      = vm->lib.nil;
    range->right     = vm->lib.nil;
    range->exclusive = 0;
    return (sl_object_t*)range;
}

static sl_range_t*
get_range(sl_vm_t* vm, SLVAL obj)
{
    sl_expect(vm, obj, vm->lib.Range);
    return (sl_range_t*)sl_get_ptr(obj);
}

typedef enum {
    ES_BEFORE,
    ES_ITERATING,
    ES_DONE
}
sl_range_enumerator_state_t;

typedef struct {
    sl_object_t base;
    SLVAL current;
    SLVAL right;
    char* method;
    sl_range_enumerator_state_t state;
}
sl_range_enumerator_t;

static sl_object_t*
allocate_range_enumerator(sl_vm_t* vm)
{
    sl_range_enumerator_t* range_enum = sl_alloc(vm->arena, sizeof(sl_range_enumerator_t));
    range_enum->current = vm->lib.nil;
    range_enum->right   = vm->lib.nil;
    range_enum->method  = NULL;
    range_enum->state   = ES_DONE;
    return (sl_object_t*)range_enum;
}

static sl_range_enumerator_t*
get_range_enumerator(sl_vm_t* vm, SLVAL obj)
{
    sl_expect(vm, obj, vm->lib.Range_Enumerator);
    return (sl_range_enumerator_t*)sl_get_ptr(obj);
}

static void
check_range_enumerator(sl_vm_t* vm, sl_range_enumerator_t* range_enum)
{
    if(sl_responds_to(vm, range_enum->current, "succ")) {
        if(sl_responds_to(vm, range_enum->current, range_enum->method)) {
            return;
        }
    }
    sl_throw_message2(vm, vm->lib.TypeError, "Uniterable type in range");
}

static SLVAL
range_init(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    sl_range_t* range = get_range(vm, self);
    range->left = argv[0];
    range->right = argv[1];
    if(argc > 2 && sl_is_truthy(argv[2])) {
        range->exclusive = 1;
    }
    return self;
}

static SLVAL
range_enumerate(sl_vm_t* vm, SLVAL self)
{
    sl_range_t* range = get_range(vm, self);
    sl_range_enumerator_t* range_enum = get_range_enumerator(vm, sl_allocate(vm, vm->lib.Range_Enumerator));
    range_enum->current     = range->left;
    range_enum->right       = range->right;
    range_enum->method      = range->exclusive ? "<" : "<=";
    range_enum->state       = ES_BEFORE;
    return sl_make_ptr((sl_object_t*)range_enum);
}

static SLVAL
range_enumerator_current(sl_vm_t* vm, SLVAL self)
{
    sl_range_enumerator_t* range_enum = get_range_enumerator(vm, self);
    check_range_enumerator(vm, range_enum);
    if(range_enum->state != ES_ITERATING) {
        sl_throw_message(vm, "Invalid operation on Range::Enumerator");
    }
    return range_enum->current;
}

static SLVAL
range_enumerator_next(sl_vm_t* vm, SLVAL self)
{
    sl_range_enumerator_t* range_enum = get_range_enumerator(vm, self);
    check_range_enumerator(vm, range_enum);
    if(range_enum->state == ES_DONE) {
        return vm->lib._false;
    }
    if(range_enum->state == ES_BEFORE) {
        range_enum->state = ES_ITERATING;
    } else {
        range_enum->current = sl_send(vm, range_enum->current, "succ", 0);
    }
    if(sl_is_truthy(sl_send(vm, range_enum->current, range_enum->method, 1, range_enum->right))) {
        return vm->lib._true;
    } else {
        range_enum->state = ES_DONE;
        return vm->lib._false;
    }
}

SLVAL
sl_make_range(sl_vm_t* vm, SLVAL lower, SLVAL upper)
{
    SLVAL rangev = sl_allocate(vm, vm->lib.Range);
    sl_range_t* range = get_range(vm, rangev);
    range->left      = lower;
    range->right     = upper;
    range->exclusive = 0;
    return rangev;
}

SLVAL
sl_make_range_exclusive(sl_vm_t* vm, SLVAL lower, SLVAL upper)
{
    SLVAL rangev = sl_allocate(vm, vm->lib.Range);
    sl_range_t* range = get_range(vm, rangev);
    range->left      = lower;
    range->right     = upper;
    range->exclusive = 1;
    return rangev;
}

void
sl_init_range(sl_vm_t* vm)
{
    vm->lib.Range = sl_define_class(vm, "Range", vm->lib.Enumerable);
    sl_class_set_allocator(vm, vm->lib.Range, allocate_range);
    sl_define_method(vm, vm->lib.Range, "init", -3, range_init);
    sl_define_method(vm, vm->lib.Range, "enumerate", 0, range_enumerate);
    
    vm->lib.Range_Enumerator = sl_define_class3(vm, sl_make_cstring(vm, "Enumerator"), vm->lib.Object, vm->lib.Range);
    sl_class_set_allocator(vm, vm->lib.Range_Enumerator, allocate_range_enumerator);
    sl_define_method(vm, vm->lib.Range_Enumerator, "current", 0, range_enumerator_current);
    sl_define_method(vm, vm->lib.Range_Enumerator, "next", 0, range_enumerator_next);
}
