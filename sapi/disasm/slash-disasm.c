#include <slash.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SECTIONS 1024

size_t section_i, section_j;
sl_vm_section_t* section_queue[MAX_SECTIONS];

void disassemble(sl_vm_t* vm, sl_vm_section_t* section);

int main(int argc, char** argv)
{
    sl_static_init();
    sl_vm_t* vm = sl_init("disasm");

    if(argc < 2) {
        fprintf(stderr, "Usage: slash-dis <source file>\n");
        exit(1);
    }

    FILE* f = fopen(argv[1], "r");
    if(!f) {
        fprintf(stderr, "Could not open %s for reading.\n", argv[1]);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* source = sl_alloc_buffer(vm->arena, size + 1);
    fread(source, size, 1, f);
    fclose(f);

    SLVAL err;
    sl_vm_frame_t frame;
    SL_TRY(frame, SL_UNWIND_ALL, {

        size_t token_count;
        sl_token_t* tokens = sl_lex(vm, (uint8_t*)argv[1], (uint8_t*)source, size, &token_count, 0);

        sl_node_base_t* ast = sl_parse(vm, tokens, token_count, (uint8_t*)argv[1]);

        sl_vm_section_t* section = sl_compile(vm, ast, (uint8_t*)argv[1]);

        disassemble(vm, section);
        while(section_j < section_i) {
            disassemble(vm, section_queue[++section_j]);
        }

    }, err, {
        if(frame.as.handler_frame.unwind_type == SL_UNWIND_EXCEPTION) {
            fprintf(stderr, "%s\n", sl_to_cstr(vm, err));
            exit(1);
        }
        if(frame.as.handler_frame.unwind_type == SL_UNWIND_EXIT) {
            int exit_code = sl_get_int(err);
            sl_free_gc_arena(vm->arena);
            exit(exit_code);
        }
    });
}

#ifdef SL_HAS_COMPUTED_GOTO
extern void*
sl_vm_op_addresses[];

static sl_vm_opcode_t
decode_opcode(void* ptr)
{
    for(sl_vm_opcode_t i = 0; i < SL_OP__MAX_OPCODE; i++) {
        if(ptr == sl_vm_op_addresses[i]) {
            return i;
        }
    }
    fprintf(stderr, "WTF: unknown opcode?\n");
    abort();
}
#endif

void disassemble(sl_vm_t* vm, sl_vm_section_t* section)
{
    size_t ip = 0, orig_ip;
    sl_vm_inline_method_cache_t* imc;

    #define NEXT() (section->insns[ip++])

    #ifdef SL_HAS_COMPUTED_GOTO
        #define NEXT_OPCODE()   (decode_opcode(NEXT().threaded_opcode))
    #else
        #define NEXT_OPCODE()   (NEXT().opcode)
    #endif

    #define NEXT_IMM()              (NEXT().imm)
    #define NEXT_UINT()             (NEXT().uint)
    #define NEXT_IMC()              (NEXT().imc)
    #define NEXT_ICC()              (NEXT().icc)
    #define NEXT_ID()               (NEXT().id)
    #define NEXT_REG_IDX()          (NEXT().reg)
    #define NEXT_SECTION()          (NEXT().section)

    #define NEXT_REG() (ctx->registers[NEXT_REG_IDX()])

    printf("%s (%p):\n", sl_to_cstr(vm, sl_id_to_string(vm, section->name)), section);

    while(ip < section->insns_count) {
        orig_ip = ip;
        switch(NEXT_OPCODE()) {
            #include "dis.inc"
        }
    }

    printf("\n");
}
