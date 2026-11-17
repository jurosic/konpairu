//compile from root

#include "../src/codegen.h"
#include <assert.h>

int main(void) {
    assert(codegen_init() == 0);
    codegen_program_start();

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

    //   if i == 2 then
    int if_id = codegen_next_label_id();
    codegen_push_var("i");
    codegen_push_int(2);
    codegen_binary_op(TOKEN_EQUALS, TYPE_INT);
    codegen_if_start(if_id);

    //     i = i - 1
    codegen_push_var("i");
    codegen_push_int(1);
    codegen_binary_op(TOKEN_SUB, TYPE_INT);
    codegen_pop_to_var("i");

    //   else
    codegen_if_else(if_id);

    //     i = i - 1
    codegen_push_var("i");
    codegen_push_int(1);
    codegen_binary_op(TOKEN_SUB, TYPE_INT);
    codegen_pop_to_var("i");

    //   end if
    codegen_if_end(if_id);

    // end while
    codegen_while_end(while_id);

    codegen_flush();
    codegen_free();
    return 0;
}
