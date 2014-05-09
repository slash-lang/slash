#include <slash/lib/range.h>
#include <slash/dispatch.h>
#include <slash/object.h>
#include <slash/class.h>
#include <slash/string.h>
#include <slash/method.h>
#include <slash/gc.h>

typedef struct {
    sl_object_t base;
    SLVAL left;
    SLVAL right;
    bool exclusive;
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
range_ptr(SLVAL obj)
{
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
    SLID method;
    sl_range_enumerator_state_t state;
}
sl_range_enumerator_t;

static sl_object_t*
allocate_range_enumerator(sl_vm_t* vm)
{
    sl_range_enumerator_t* range_enum = sl_alloc(vm->arena, sizeof(sl_range_enumerator_t));
    range_enum->current = vm->lib.nil;
    range_enum->right   = vm->lib.nil;
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
    if(sl_responds_to2(vm, range_enum->current, vm->id.succ)) {
        if(sl_responds_to2(vm, range_enum->current, range_enum->method)) {
            return;
        }
    }
    sl_throw_message2(vm, vm->lib.TypeError, "Uniterable type in range");
}

static SLVAL
range_inclusive_init(sl_vm_t* vm, SLVAL self, SLVAL left, SLVAL right)
{
    range_ptr(self)->left = left;
    range_ptr(self)->right = right;
    range_ptr(self)->exclusive = 0;
    return self;
    (void)vm;
}

static SLVAL
range_exclusive_init(sl_vm_t* vm, SLVAL self, SLVAL left, SLVAL right)
{
    range_ptr(self)->left = left;
    range_ptr(self)->right = right;
    range_ptr(self)->exclusive = 1;
    return self;
    (void)vm;
}

static SLVAL
range_enumerate(sl_vm_t* vm, SLVAL self)
{
    sl_range_enumerator_t* range_enum = get_range_enumerator(vm, sl_allocate(vm, vm->lib.Range_Enumerator));
    range_enum->current     = range_ptr(self)->left;
    range_enum->right       = range_ptr(self)->right;
    range_enum->method      = range_ptr(self)->exclusive ? vm->id.op_lt : vm->id.op_lte;
    range_enum->state       = ES_BEFORE;
    return sl_make_ptr((sl_object_t*)range_enum);
}

static SLVAL
range_enumerator_current(sl_vm_t* vm, SLVAL self)
{
    sl_range_enumerator_t* range_enum = get_range_enumerator(vm, self);
    check_range_enumerator(vm, range_enum);
    if(range_enum->state != ES_ITERATING) {
        sl_throw_message2(vm, vm->lib.TypeError, "Invalid operation on Range::Enumerator");
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
        range_enum->current = sl_send_id(vm, range_enum->current, vm->id.succ, 0);
    }
    if(sl_is_truthy(sl_send_id(vm, range_enum->current, range_enum->method, 1, range_enum->right))) {
        return vm->lib._true;
    } else {
        range_enum->state = ES_DONE;
        return vm->lib._false;
    }
}

SLVAL
sl_make_range_inclusive(sl_vm_t* vm, SLVAL lower, SLVAL upper)
{
    SLVAL range = sl_allocate(vm, vm->lib.Range_Inclusive);
    range_ptr(range)->left      = lower;
    range_ptr(range)->right     = upper;
    range_ptr(range)->exclusive = 0;
    return range;
}

SLVAL
sl_make_range_exclusive(sl_vm_t* vm, SLVAL lower, SLVAL upper)
{
    SLVAL range = sl_allocate(vm, vm->lib.Range_Exclusive);
    range_ptr(range)->left      = lower;
    range_ptr(range)->right     = upper;
    range_ptr(range)->exclusive = 1;
    return range;
}

SLVAL
sl_range_lower(sl_vm_t* vm, SLVAL range)
{
    return range_ptr(range)->left;
    (void)vm;
}

SLVAL
sl_range_upper(sl_vm_t* vm, SLVAL range)
{
    return range_ptr(range)->right;
    (void)vm;
}

bool
sl_range_is_exclusive(sl_vm_t* vm, SLVAL range)
{
    return range_ptr(range)->exclusive;
    (void)vm;
}

static SLVAL
range_inclusive_inspect(sl_vm_t* vm, SLVAL range)
{
    return sl_make_formatted_string(vm, "%V..%V", range_ptr(range)->left, range_ptr(range)->right);
}

static SLVAL
range_exclusive_inspect(sl_vm_t* vm, SLVAL range)
{
    return sl_make_formatted_string(vm, "%V...%V", range_ptr(range)->left, range_ptr(range)->right);
}

void
sl_init_range(sl_vm_t* vm)
{
    SLVAL Range = sl_define_class(vm, "Range", vm->lib.Object);

    vm->lib.Range_Inclusive = sl_define_class3(vm, sl_intern(vm, "Inclusive"), vm->lib.Enumerable, Range);
    sl_class_set_allocator(vm, vm->lib.Range_Inclusive, allocate_range);
    sl_define_method(vm, vm->lib.Range_Inclusive, "init", 2, range_inclusive_init);
    sl_define_method(vm, vm->lib.Range_Inclusive, "enumerate", 0, range_enumerate);
    sl_define_method(vm, vm->lib.Range_Inclusive, "lower", 0, sl_range_lower);
    sl_define_method(vm, vm->lib.Range_Inclusive, "upper", 0, sl_range_upper);
    sl_define_method(vm, vm->lib.Range_Inclusive, "inspect", 0, range_inclusive_inspect);

    vm->lib.Range_Exclusive = sl_define_class3(vm, sl_intern(vm, "Exclusive"), vm->lib.Enumerable, Range);
    sl_class_set_allocator(vm, vm->lib.Range_Exclusive, allocate_range);
    sl_define_method(vm, vm->lib.Range_Exclusive, "init", 2, range_exclusive_init);
    sl_define_method(vm, vm->lib.Range_Exclusive, "enumerate", 0, range_enumerate);
    sl_define_method(vm, vm->lib.Range_Exclusive, "lower", 0, sl_range_lower);
    sl_define_method(vm, vm->lib.Range_Exclusive, "upper", 0, sl_range_upper);
    sl_define_method(vm, vm->lib.Range_Exclusive, "inspect", 0, range_exclusive_inspect);

    vm->lib.Range_Enumerator = sl_define_class3(vm, sl_intern(vm, "Enumerator"), vm->lib.Object, Range);
    sl_class_set_allocator(vm, vm->lib.Range_Enumerator, allocate_range_enumerator);
    sl_define_method(vm, vm->lib.Range_Enumerator, "current", 0, range_enumerator_current);
    sl_define_method(vm, vm->lib.Range_Enumerator, "next", 0, range_enumerator_next);
}
