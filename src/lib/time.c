#include <slash/vm.h>
#include <slash/class.h>
#include <slash/object.h>
#include <slash/string.h>
#define __USE_POSIX
#include <time.h>

/*
 * This is my really basic and trivial time handling code. It has no bells or
 * whistles and doesn't bother handling stuff like leap seconds. I don't know
 * all that much about the intracacies of date and time so this code should
 * probably be replaced with something more robust at some point.
 */

typedef struct {
    sl_object_t base;
    time_t tm;
}
sl_time_t;

static sl_object_t*
time_allocate(sl_vm_t* vm)
{
    return sl_alloc(vm->arena, sizeof(sl_time_t));
}

static sl_time_t*
get_time(sl_vm_t* vm, SLVAL val)
{
    sl_expect(vm, val, vm->lib.Time);
    return (sl_time_t*)sl_get_ptr(val);
}

static SLVAL
time_now(sl_vm_t* vm)
{
    SLVAL self = sl_allocate(vm, vm->lib.Time);
    get_time(vm, self)->tm = time(NULL);
    return self;
}

static struct tm
read_time_args(sl_vm_t* vm, size_t argc, SLVAL* argv)
{
    struct tm tm = {
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 0,
        .tm_mon = 0,
        .tm_year = 0,
    };
    switch(argc) {
        default:
        case 6:
            tm.tm_sec = sl_get_int(sl_expect(vm, argv[5], vm->lib.Int));
        case 5:
            tm.tm_min = sl_get_int(sl_expect(vm, argv[4], vm->lib.Int));
        case 4:
            tm.tm_hour = sl_get_int(sl_expect(vm, argv[3], vm->lib.Int));
        case 3:
            tm.tm_mday = sl_get_int(sl_expect(vm, argv[2], vm->lib.Int));
        case 2:
            tm.tm_mon = sl_get_int(sl_expect(vm, argv[1], vm->lib.Int)) - 1;
        case 1:
            tm.tm_year = sl_get_int(sl_expect(vm, argv[0], vm->lib.Int)) - 1900;
        case 0:
            break;
    }
    return tm;
}

static time_t
tz_offset()
{
    #ifdef SL_HAS_THREADSAFE_TIME
        time_t ts = time(NULL);
        struct tm local_now;
        localtime_r(&ts, &local_now);
        time_t local_ts = mktime(&local_now);
        struct tm utc_now;
        gmtime_r(&ts, &utc_now);
        time_t utc_ts = mktime(&utc_now);
        return local_ts - utc_ts;
    #else
        time_t ts = time(NULL);
        time_t local_ts = mktime(localtime(&ts));
        time_t utc_ts = mktime(gmtime(&ts));
        return local_ts - utc_ts;
    #endif
}

static SLVAL
time_local(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    SLVAL obj = sl_allocate(vm, vm->lib.Time);
    struct tm tm = read_time_args(vm, argc, argv);
    get_time(vm, obj)->tm = mktime(&tm);
    return obj;
    (void)self;
}

static SLVAL
time_clock(sl_vm_t* vm)
{
    return sl_make_int(vm, clock());
}

static SLVAL
time_init(sl_vm_t* vm, SLVAL self, size_t argc, SLVAL* argv)
{
    struct tm tm = read_time_args(vm, argc, argv);
    time_t local = mktime(&tm);
    time_t utc = local + tz_offset();
    get_time(vm, self)->tm = utc;
    return self;
}

static SLVAL
time_to_s(sl_vm_t* vm, SLVAL self)
{
    char time_str[128];
    time_t utc_time = get_time(vm, self)->tm;
    char* fmt = "%a %b %d %Y %X UTC";
    #ifdef SL_HAS_THREADSAFE_TIME
        struct tm tm;
        gmtime_r(&utc_time, &tm);
        strftime(time_str, 1023, fmt, &tm);
    #else
        strftime(time_str, 1023, fmt, gmtime(&utc_time));
    #endif
    return sl_make_cstring(vm, time_str);
}

static SLVAL
time_add(sl_vm_t* vm, SLVAL self, SLVAL offset)
{
    sl_expect(vm, offset, vm->lib.Int);
    SLVAL new_obj = sl_allocate(vm, vm->lib.Time);
    get_time(vm, new_obj)->tm = get_time(vm, self)->tm + sl_get_int(offset);
    return new_obj;
}

static SLVAL
time_sub(sl_vm_t* vm, SLVAL self, SLVAL other)
{
    if(sl_is_a(vm, other, vm->lib.Time)) {
        return sl_make_int(vm, get_time(vm, self)->tm - get_time(vm, other)->tm);
    }
    sl_expect(vm, other, vm->lib.Int);
    SLVAL new_obj = sl_allocate(vm, vm->lib.Time);
    get_time(vm, new_obj)->tm = get_time(vm, self)->tm - sl_get_int(other);
    return new_obj;
}

static SLVAL
time_strftime(sl_vm_t* vm, SLVAL self, SLVAL format)
{
    struct tm* tm_ptr;
    size_t capacity = 256;
    char* fmt = sl_to_cstr(vm, format);
    char* buff = sl_alloc_buffer(vm->arena, capacity);
    tm_ptr = gmtime(&get_time(vm, self)->tm);
    while(strftime(buff, capacity, fmt, tm_ptr) == 0) {
        capacity *= 2;
        buff = sl_realloc(vm->arena, buff, capacity);
    }
    return sl_make_cstring(vm, buff);
}

void
sl_init_time(sl_vm_t* vm)
{
    vm->lib.Time = sl_define_class(vm, "Time", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.Time, time_allocate);
    sl_define_singleton_method(vm, vm->lib.Time, "now", 0, time_now);
    sl_define_singleton_method(vm, vm->lib.Time, "local", -1, time_local);
    sl_define_singleton_method(vm, vm->lib.Time, "clock", 0, time_clock);
    sl_class_set_const(vm, vm->lib.Time, "CLOCKS_PER_SEC", sl_make_int(vm, CLOCKS_PER_SEC));
    sl_define_method(vm, vm->lib.Time, "init", -1, time_init);
    sl_define_method(vm, vm->lib.Time, "to_s", 0, time_to_s);
    sl_define_method(vm, vm->lib.Time, "+", 1, time_add);
    sl_define_method(vm, vm->lib.Time, "-", 1, time_sub);
    sl_define_method(vm, vm->lib.Time, "strftime", 1, time_strftime);
}