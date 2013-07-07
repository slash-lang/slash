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
#include <slash/mem.h>

static int
sl_statically_initialized;

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

#define LIB(lib) void sl_init_##lib(sl_vm_t*)
#define LIB_INIT(lib) sl_init_##lib(vm)

LIB(method);

LIB(true);
LIB(false);
LIB(nil);
LIB(comparable);
LIB(number);
LIB(int);
LIB(float);
LIB(bignum);
LIB(method);
LIB(enumerable);
LIB(array);
LIB(require);
LIB(lambda);
LIB(file);
LIB(dict);
LIB(rand);
LIB(request);
LIB(response);
LIB(system);
LIB(regexp);
LIB(range);
LIB(time);
LIB(gc);
LIB(eval);
LIB(buffer);
LIB(version);

void sl_init_exts(sl_vm_t* vm);

static void
sl_init_libs(sl_vm_t* vm)
{
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
    LIB_INIT(version);

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
    vm->store = sl_st_init_numtable(vm);
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

    sl_pre_init_class(vm);
    sl_pre_init_object(vm);
    sl_pre_init_string(vm);

    Object = (sl_class_t*)sl_get_ptr(vm->lib.Object);
    Class = (sl_class_t*)sl_get_ptr(vm->lib.Class);
    String = (sl_class_t*)sl_get_ptr(vm->lib.String);
    Object->name = sl_intern(vm, "Object");
    Class->name = sl_intern(vm, "Class");
    String->name = sl_intern(vm, "String");

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

SLVAL
sl_vm_store_get(sl_vm_t* vm, void* key)
{
    SLVAL val = vm->lib.nil;
    sl_st_lookup(vm->store, (sl_st_data_t)key, (sl_st_data_t*)&val);
    return val;
}

void
sl_vm_store_put(sl_vm_t* vm, void* key, SLVAL val)
{
    sl_st_insert(vm->store, (sl_st_data_t)key, (sl_st_data_t)sl_get_ptr(val));
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
