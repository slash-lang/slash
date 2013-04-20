#include <slash.h>
#include <ruby.h>
#include <ruby/version.h>
#include <ruby/encoding.h>
#include <pthread.h>
#include <signal.h>

static pthread_mutex_t
sl_ruby_lock;

static rb_encoding*
sl_ruby_utf8_enc;

static int
cRuby_Object,
cRuby_Exception;

typedef struct {
    sl_object_t base;
    VALUE obj;
}
sl_ruby_object_t;

static void
free_ruby_object(void* vobj)
{
    sl_ruby_object_t* obj = vobj;
    rb_gc_unregister_address(&obj->obj);
}

static sl_object_t*
alloc_ruby_object(sl_vm_t* vm)
{
    sl_ruby_object_t* obj = sl_alloc(vm->arena, sizeof(sl_ruby_object_t));
    obj->obj = Qnil;
    rb_gc_register_address(&obj->obj);
    sl_gc_set_finalizer(obj, free_ruby_object);
    return (sl_object_t*)obj;
}

static sl_ruby_object_t*
get_ruby_object(sl_vm_t* vm, SLVAL obj)
{
    SLVAL Ruby_Object = sl_vm_store_get(vm, &cRuby_Object);
    sl_expect(vm, obj, Ruby_Object);
    return (sl_ruby_object_t*)sl_get_ptr(obj);
}

static SLVAL
make_ruby_object(sl_vm_t* vm, VALUE ruby_value)
{
    SLVAL Ruby_Object = sl_vm_store_get(vm, &cRuby_Object);
    SLVAL robj = sl_allocate(vm, Ruby_Object);
    ((sl_ruby_object_t*)sl_get_ptr(robj))->obj = ruby_value;
    return robj;
}

struct sl_ruby_protect_args {
    sl_vm_t* vm;
    void* data;
    void(*func)(sl_vm_t*,void*);
    VALUE exception;
    SLVAL sl_exception;
};

static VALUE
sl_ruby_protect_try(struct sl_ruby_protect_args* args)
{
    args->func(args->vm, args->data);
    return 0;
}

static VALUE
sl_ruby_protect_catch(struct sl_ruby_protect_args* args, VALUE exception)
{
    args->exception = exception;
    return 0;
}

static SLVAL
sl_ruby_to_slash(sl_vm_t* vm, VALUE ruby_value);

static void
sl_ruby_protect(sl_vm_t* vm, void(*func)(sl_vm_t*,void*), void* data)
{
    struct sl_ruby_protect_args args = { vm, data, func, 0, { 0 } };
    sl_vm_frame_t frame;
    pthread_mutex_lock(&sl_ruby_lock);
    SL_ENSURE(frame, {
        rb_rescue2(sl_ruby_protect_try, (VALUE)&args, sl_ruby_protect_catch, (VALUE)&args, rb_eException, 0);
        pthread_mutex_unlock(&sl_ruby_lock);
        if(args.exception) {
            SLVAL Ruby_Exception = sl_vm_store_get(vm, &cRuby_Exception);
            SLVAL err = sl_allocate(vm, Ruby_Exception);
            sl_error_set_message(vm, err, sl_ruby_to_slash(vm, rb_obj_as_string(args.exception)));
            sl_set_ivar(vm, err, sl_intern(vm, "object"), make_ruby_object(vm, args.exception));
            args.sl_exception = err;
        }
    }, {
        pthread_mutex_unlock(&sl_ruby_lock);
    });
    if(args.exception) {
        sl_throw(vm, args.sl_exception);
    }
}

static SLVAL
sl_ruby_to_slash(sl_vm_t* vm, VALUE ruby_value)
{
    switch(TYPE(ruby_value)) {
        case T_NIL:
            return vm->lib.nil;
        case T_TRUE:
            return vm->lib._true;
        case T_FALSE:
            return vm->lib._false;
        case T_FIXNUM:
            return sl_make_int(vm, FIX2INT(ruby_value));
        case T_FLOAT:
            return sl_make_float(vm, rb_float_value(ruby_value));
        case T_BIGNUM:
            sl_throw_message2(vm, vm->lib.TypeError, "bignum returned from ruby");
        case T_STRING:
            ruby_value = rb_str_export_to_enc(ruby_value, sl_ruby_utf8_enc);
            return sl_make_string(vm, (uint8_t*)RSTRING_PTR(ruby_value), RSTRING_LEN(ruby_value));
        default:
            return make_ruby_object(vm, ruby_value);
    }
    return vm->lib.nil;
}

static VALUE
sl_slash_to_ruby(sl_vm_t* vm, SLVAL slash_value)
{
    switch(sl_get_primitive_type(slash_value)) {
        case SL_T_NIL:
            return Qnil;
        case SL_T_FALSE:
            return Qfalse;
        case SL_T_TRUE:
            return Qtrue;
        case SL_T_STRING: {
            sl_string_t* str = sl_get_string(vm, slash_value);
            return rb_enc_str_new((char*)str->buff, str->buff_len, sl_ruby_utf8_enc);
        }
        case SL_T_INT:
            return INT2FIX(sl_get_int(slash_value));
        case SL_T_FLOAT:
            return DBL2NUM(sl_get_float(vm, slash_value));
        case SL_T_BIGNUM:
            sl_throw_message2(vm, vm->lib.TypeError, "bignum passed to ruby");
        default:
            sl_throw_message2(vm, vm->lib.TypeError, "dunno how to work with this value passed to ruby");
    }
    return Qundef;
}

struct sl_ruby_eval_args {
    char* cstr;
    SLVAL ret;
};

static void
locked_sl_ruby_eval(sl_vm_t* vm, void* vargs)
{
    struct sl_ruby_eval_args* args = vargs;
    args->ret = sl_ruby_to_slash(vm, rb_eval_string(args->cstr));
}

static SLVAL
sl_ruby_eval(sl_vm_t* vm, SLVAL self, SLVAL str)
{
    struct sl_ruby_eval_args args;
    args.cstr = sl_to_cstr(vm, sl_expect(vm, str, vm->lib.String));
    sl_ruby_protect(vm, locked_sl_ruby_eval, &args);
    return args.ret;
    (void)self;
}

struct sl_ruby_object_inspect_args {
    SLVAL self;
    SLVAL ret;
};

static void
locked_sl_ruby_object_inspect(sl_vm_t* vm, void* vargs)
{
    struct sl_ruby_object_inspect_args* args = vargs;
    VALUE ruby_inspected = rb_inspect(get_ruby_object(vm, args->self)->obj);
    SLVAL slash_inspected = sl_make_string(vm, (uint8_t*)RSTRING_PTR(inspected), RSTRING_LEN(inspected));

    args->ret = sl_make_formatted_string(vm, "#<Ruby: %V>", slash_inspected);
}

static SLVAL
sl_ruby_object_inspect(sl_vm_t* vm, SLVAL self)
{
    struct sl_ruby_object_inspect_args args = { self, { 0 } };
    sl_ruby_protect(vm, locked_sl_ruby_object_inspect, &args);
    return args.ret;
}

struct sl_ruby_object_ruby_send_args {
    SLVAL self;
    size_t argc;
    SLVAL* argv;
    SLVAL ret;
};

static void
locked_sl_ruby_object_ruby_send(sl_vm_t* vm, void* vargs)
{
    struct sl_ruby_object_ruby_send_args* args = vargs;
    sl_ruby_object_t* recv = get_ruby_object(vm, args->self);
    char* method = sl_to_cstr(vm, sl_expect(vm, args->argv[0], vm->lib.String));
    args->argc--;
    args->argv++;
    VALUE ruby_argv[args->argc];
    for(size_t i = 0; i < args->argc; i++) {
        ruby_argv[i] = sl_slash_to_ruby(vm, args->argv[i]);
    }
    VALUE ret = rb_funcall2(recv->obj, rb_intern(method), (int)args->argc, ruby_argv);
    args->ret = sl_ruby_to_slash(vm, ret);
}

static SLVAL
sl_ruby_object_ruby_send(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    struct sl_ruby_object_ruby_send_args args = { self, argc, argv, { 0 } };
    sl_ruby_protect(vm, locked_sl_ruby_object_ruby_send, &args);
    return args.ret;
}

static SLVAL
sl_ruby_exception_object(sl_vm_t* vm, SLVAL object)
{
    return sl_get_ivar(vm, object, sl_intern(vm, "object"));
}

void
sl_init_ext_ruby(sl_vm_t* vm)
{
    SLVAL Ruby = sl_define_class(vm, "Ruby", vm->lib.Object);
    sl_define_singleton_method(vm, Ruby, "eval", 1, sl_ruby_eval);
    sl_define_singleton_method(vm, Ruby, "[]", 1, sl_ruby_eval);
    sl_class_set_const(vm, Ruby, "VERSION", sl_make_cstring(vm, ruby_version));
    sl_class_set_const(vm, Ruby, "DESCRIPTION", sl_make_cstring(vm, ruby_description));

    SLVAL Ruby_Object = sl_define_class3(vm, sl_intern(vm, "Object"), vm->lib.Object, Ruby);
    sl_class_set_allocator(vm, Ruby_Object, alloc_ruby_object);
    sl_define_method(vm, Ruby_Object, "inspect", 0, sl_ruby_object_inspect);
    sl_define_method(vm, Ruby_Object, "ruby_send", -2, sl_ruby_object_ruby_send);
    sl_vm_store_put(vm, &cRuby_Object, Ruby_Object);

    SLVAL Ruby_Exception = sl_define_class3(vm, sl_intern(vm, "Exception"), vm->lib.Error, Ruby);
    sl_define_method(vm, Ruby_Exception, "object", 0, sl_ruby_exception_object);
    sl_vm_store_put(vm, &cRuby_Exception, Ruby_Exception);
}

void Init_enc();

#define SIGNAL_COUNT_ASSUMPTION 32

static int
saved_signals[SIGNAL_COUNT_ASSUMPTION];

static void
save_signals()
{
    for(int i = 0; i < SIGNAL_COUNT_ASSUMPTION; i++) {
        saved_signals[i] = signal(i, SIG_DFL);
    }
}

static void
restore_signals()
{
    for(int i = 0; i < SIGNAL_COUNT_ASSUMPTION; i++) {
        if(saved_signals[i] != SIG_ERR) {
            signal(i, saved_signals[i]);
        }
    }
}

void
sl_static_init_ext_ruby()
{
    pthread_mutex_init(&sl_ruby_lock, NULL);

    // ruby likes to clobber signal handlers on init, but that doesn't work
    // well when we embed it. we'll take the old signal handlers, save them,
    // call ruby_init, then restore the old signal handlers
    save_signals();
    ruby_init();
    restore_signals();
    ruby_init_loadpath();
    Init_enc();
    sl_ruby_utf8_enc = rb_enc_find("UTF-8");
}
