#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <slash/lib/rand.h>
#include <slash/value.h>
#include <slash/vm.h>
#include <slash/string.h>
#include <slash/class.h>
#include <slash/object.h>
#include <slash/error.h>
#include <slash/platform.h>
#include <slash/method.h>
#include <slash/gc.h>

static int
sl_statically_initialized;

static int
vm_store_slot_count = 0;

void
sl_static_init_exts();

void
sl_static_init_vm_internals();

void
sl_static_init()
{
    if(sl_statically_initialized) {
        return;
    }
    sl_statically_initialized = 1;
    sl_static_init_exts();
    sl_static_init_vm_internals();
}

void sl_init_exts(sl_vm_t* vm);

static void
sl_init_libs(sl_vm_t* vm)
{
    #define LIB_INIT(lib) do { void sl_init_##lib(sl_vm_t*); sl_init_##lib(vm); } while(0)
    LIB_INIT(nil);
    LIB_INIT(comparable);
    LIB_INIT(number);
    LIB_INIT(int);
    LIB_INIT(float);
    LIB_INIT(bignum);
    LIB_INIT(true);
    LIB_INIT(false);
    LIB_INIT(enumerable);
    LIB_INIT(array);
    LIB_INIT(require);
    LIB_INIT(lambda);
    LIB_INIT(file);
    LIB_INIT(dict);
    LIB_INIT(rand);
    LIB_INIT(request);
    LIB_INIT(response);
    LIB_INIT(system);
    LIB_INIT(regexp);
    LIB_INIT(range);
    LIB_INIT(time);
    LIB_INIT(gc);
    LIB_INIT(eval);
    LIB_INIT(buffer);
    LIB_INIT(slash);
    #undef LIB_INIT

    sl_init_exts(vm);
}

static void
sl_init_id(sl_vm_t* vm);

sl_vm_t*
sl_init(const char* sapi_name)
{
    sl_vm_t* vm;
    sl_class_t* Object;
    sl_class_t* Class;
    sl_class_t* String;
    sl_gc_arena_t* arena = sl_make_gc_arena();

    sl_gc_disable(arena);
    vm = sl_alloc(arena, sizeof(sl_vm_t));
    vm->arena = arena;
    vm->cwd = ".";
    vm->initializing = 1;
    vm->hash_seed = rand();
    vm->stack_limit = sl_stack_limit();
    vm->required = sl_st_init_table(vm, &sl_string_hash_type);
    vm->call_stack = NULL;

    vm->intern.name_to_id = sl_st_init_table(vm, &sl_string_hash_type);
    vm->intern.id_to_name_cap = 32;
    vm->intern.id_to_name_size = 0;
    vm->intern.id_to_name = sl_alloc(vm->arena, sizeof(SLVAL) * vm->intern.id_to_name_cap);

    vm->lib.nil = sl_make_ptr(sl_alloc(arena, sizeof(sl_object_t)));
    vm->lib.Object = sl_make_ptr(sl_alloc(arena, sizeof(sl_class_t)));

    vm->store = sl_alloc(arena, sizeof(SLVAL) * vm_store_slot_count);
    for(int i = 0; i < vm_store_slot_count; i++) {
        vm->store[i] = vm->lib.nil;
    }

    sl_pre_init_class(vm);
    sl_pre_init_object(vm);
    sl_pre_init_string(vm);

    Object = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    Class = (sl_class_t*)sl_get_ptr(vm->lib.Class);
    String = (sl_class_t*)sl_get_ptr(vm->lib.String);
    Object->extra->name = sl_intern(vm, "Object");
    Class->extra->name = sl_intern(vm, "Class");
    String->extra->name = sl_intern(vm, "String");

    sl_init_id(vm);

    sl_init_method(vm);
    sl_init_class(vm);
    sl_init_object(vm);
    sl_init_string(vm);
    sl_init_error(vm);

    sl_init_libs(vm);

    vm->initializing = 0;

    String->super = vm->lib.Comparable;

    vm->lib.StackOverflowError_instance = sl_make_error2(vm, vm->lib.StackOverflowError, sl_make_cstring(vm, "Stack Overflow"));

    vm->state_constant = 1;
    vm->state_method = 1;

    sl_class_set_const(vm, vm->lib.Object, "SAPI", sl_make_cstring(vm, sapi_name));

    sl_gc_enable(arena);
    return vm;
}

int
sl_vm_store_register_slot()
{
    return vm_store_slot_count++;
}

static void
sl_init_id(sl_vm_t* vm)
{
    #define ID(name)        vm->id.name = sl_intern(vm, #name)
    #define OP(name, cstr)  vm->id.op_##name = sl_intern(vm, cstr)

    OP(cmp, "<=>");
    OP(eq,  "==");
    OP(lt,  "<");
    OP(lte, "<=");
    OP(add, "+");
    OP(sub, "-");
    OP(div, "/");

    ID(call);
    ID(current);
    ID(enumerate);
    ID(hash);
    ID(init);
    ID(inspect);
    ID(method_missing);
    ID(next);
    ID(succ);
    ID(to_s);
    ID(to_f);
    ID(subclassed);
}
