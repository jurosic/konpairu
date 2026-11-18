//compile from root

#include "../src/codegen.h"
#include <assert.h>

int main(void) {
    assert(codegen_init() == 0);
    codegen_program_start();

    codegen_main_start();

    // Scope 1: val x = 10
    codegen_def_var("x", 1);
    codegen_push_int(10);
    codegen_pop_to_var("x", 1);

    // if x > 0 then
    int if_id = codegen_next_label_id();
    codegen_push_var("x", 1);
    codegen_push_int(0);
    codegen_binary_op(TOKEN_GREATER, TYPE_INT);
    codegen_if_start(if_id);

    //   Scope 2: val x = 20
    codegen_def_var("x", 2);
    codegen_push_int(20);
    codegen_pop_to_var("x", 2);

    //   print(x) -> 20
    codegen_push_var("x", 2);
    codegen_builtin_print(TYPE_INT);
    codegen_push_string("\n");
    codegen_builtin_print(TYPE_STRING);

    // else ()
    codegen_if_else(if_id);
    codegen_if_end(if_id);

    // Back in Scope 1: print(x) -> 10
    codegen_push_var("x", 1);
    codegen_builtin_print(TYPE_INT);
    codegen_push_string("\n");
    codegen_builtin_print(TYPE_STRING);

    codegen_main_end();

    codegen_flush();
    codegen_free();
    return 0;
}
