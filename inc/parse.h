#ifndef SL_PARSE_H
#define SL_PARSE_H

#include "vm.h"
#include "lex.h"
#include "ast.h"

typedef struct {
    sl_vm_t* vm;
    sl_token_t* tokens;
    size_t token_count;
    size_t current_token;
    uint8_t* filename;
    sl_node_base_t* ast;
}
sl_parse_state_t;

sl_node_base_t*
sl_parse(sl_vm_t* vm, sl_token_t* tokens, size_t token_count, uint8_t* filename);

sl_node_seq_t*
sl_make_seq_node();

void
sl_seq_node_append(sl_node_seq_t* seq, sl_node_base_t* node);

sl_node_base_t*
sl_make_raw_node(sl_parse_state_t* ps, sl_token_t* token);

sl_node_base_t*
sl_make_echo_node(sl_node_base_t* expr);

sl_node_base_t*
sl_make_raw_echo_node(sl_node_base_t* expr);

#endif
