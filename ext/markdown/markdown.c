#include <mkdio.h>
#include <slash.h>

static SLVAL
markdown_compile(sl_vm_t* vm, SLVAL self, SLVAL mkd)
{
    sl_expect(vm, mkd, vm->lib.String);
    sl_string_t* ptr = (sl_string_t*)sl_get_ptr(mkd);
    
    MMIOT* doc = mkd_string((char*)ptr->buff, ptr->buff_len, 0);
    if(!mkd_compile(doc, 0)) {
        mkd_cleanup(doc);
        sl_throw_message2(vm, vm->lib.SyntaxError, "Invalid Markdown input");
    }
    
    char* output;
    int len = mkd_document(doc, &output);
    SLVAL retn = sl_make_string(vm, (uint8_t*)output, len);
    mkd_cleanup(doc);
    return retn;
    
    (void)self;
}

void
sl_init_ext_markdown(sl_vm_t* vm)
{
    SLVAL Markdown = sl_define_class(vm, "Markdown", vm->lib.Object);
    sl_define_singleton_method(vm, Markdown, "compile", 1, markdown_compile);
}