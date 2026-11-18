//compile from root

#include "../src/codegen.h"
#include <assert.h>

int main(void) {
    assert(codegen_init() == 0);
    codegen_program_start();

    codegen_main_start();

    // var i = 3
    codegen_def_var("i");
    codegen_push_int(3);
    codegen_pop_to_var("i");

    // while i > 0 do
    int while_id = codegen_next_label_id();
    codegen_while_header(while_id);
    codegen_push_var("i");
    codegen_push_int(0);
    codegen_binary_op(TOKEN_GREATER, TYPE_INT);
    codegen_while_body_start(while_id);

    //   var loop_var = i * 10  (declared inside loop, hoisted safely to top!)
    codegen_def_var("loop_var");
    codegen_push_var("i");
    codegen_push_int(10);
    codegen_binary_op(TOKEN_MUL, TYPE_INT);
    codegen_pop_to_var("loop_var");

    //   if i == 2 then i = i - 1 else i = i - 1
    int if_id = codegen_next_label_id();
    codegen_push_var("i");
    codegen_push_int(2);
    codegen_binary_op(TOKEN_EQUALS, TYPE_INT);
    codegen_if_start(if_id);

    codegen_push_var("i");
    codegen_push_int(1);
    codegen_binary_op(TOKEN_SUB, TYPE_INT);
    codegen_pop_to_var("i");

    codegen_if_else(if_id);

    codegen_push_var("i");
    codegen_push_int(1);
    codegen_binary_op(TOKEN_SUB, TYPE_INT);
    codegen_pop_to_var("i");

    codegen_if_end(if_id);
    codegen_while_end(while_id);

    codegen_main_end();

    codegen_flush();
    codegen_free();
    return 0;
}
