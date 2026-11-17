//compile from root

#include "../src/codegen.h"
#include <assert.h>

int main(void) {
    assert(codegen_init() == 0);
    codegen_program_start();

    // val x: Double = 5 + 2.5
    codegen_def_var("x");
    codegen_push_int(5);
    codegen_push_double(2.5);
    codegen_int2float_second(); // semantics.c told us convert_left = true
    codegen_binary_op(TOKEN_ADD, TYPE_DOUBLE);
    codegen_pop_to_var("x");

    // val msg: String = "hi " + "there"
    codegen_def_var("msg");
    codegen_push_string("hi ");
    codegen_push_string("there");
    codegen_binary_op(TOKEN_ADD, TYPE_STRING);
    codegen_pop_to_var("msg");

    codegen_flush();
    codegen_free();
    return 0;
}
