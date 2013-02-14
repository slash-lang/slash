#define OPT(opt, _) sl_class_set_const(vm, Curl, #opt, sl_make_int(vm, opt))
#define CONST(opt)  sl_class_set_const(vm, Curl, #opt, sl_make_int(vm, opt))

static void
sl_curlopts_init(sl_vm_t* vm, SLVAL Curl)
{
    #include "curlopts.inc"
}

#undef OPT
#undef CONST
