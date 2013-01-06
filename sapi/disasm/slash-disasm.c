#include <slash.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SECTIONS 1024

sl_vm_t* vm;

size_t section_i, section_j;
sl_vm_section_t* section_queue[MAX_SECTIONS];

void disassemble(sl_vm_section_t* section);

int main(int argc, char** argv)
{
    sl_static_init();
    vm = sl_init();
    
    if(argc < 1) {
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
    sl_catch_frame_t frame;
    SL_TRY(frame, SL_UNWIND_ALL, {
        
        size_t token_count;
        sl_token_t* tokens = sl_lex(vm, (uint8_t*)argv[1], source, size, &token_count, 0);
        
        sl_node_base_t* ast = sl_parse(vm, tokens, token_count, (uint8_t*)argv[1]);
        
        sl_vm_section_t* section = sl_compile(vm, ast, (uint8_t*)argv[1]);

        disassemble(section);
        while(section_j < section_i) {
            disassemble(section_queue[++section_j]);
        }
        
    }, err, {
        if(frame.type == SL_UNWIND_EXCEPTION) {
            fprintf(stderr, "%s\n", sl_to_cstr(vm, err));
            exit(1);
        }
        if(frame.type == SL_UNWIND_EXIT) {
            int exit_code = sl_get_int(err);
            sl_free_gc_arena(vm->arena);
            exit(exit_code);
        }
    });
}

void disassemble(sl_vm_section_t* section)
{
    size_t ip = 0;
    
    #define NEXT() (section->insns[ip++])

    #define NEXT_IMM() (NEXT().imm)
    #define NEXT_UINT() (NEXT().uint)
    #define NEXT_PTR() ((void*)NEXT().uint)
    #define NEXT_ID() (NEXT().id)
    
    printf("%s (%p):\n", sl_to_cstr(vm, sl_id_to_string(vm, section->name)), section);
    
    while(ip < section->insns_count) {
        switch(NEXT().opcode) {
            #include "dis.inc"
        }
    }
    
    printf("\n");
}
