//compile from root

#include "../src/codegen.h"
#include <assert.h>

int main(void) {
    assert(codegen_init() == 0);
    codegen_program_start();

    // ==========================================
    // def fib(n: Int): Int =
    // ==========================================
    codegen_func_start("fib");
    codegen_func_param("n");

    // if n < 2 then
    int if_id = codegen_next_label_id();
    codegen_push_var("n");
    codegen_push_int(2);
    codegen_binary_op(TOKEN_LESS, TYPE_INT);
    codegen_if_start(if_id);

    //     return n
    codegen_push_var("n");
    codegen_func_return();

    // else
    codegen_if_else(if_id);

    //     val a = n - 1
    codegen_def_var("a");
    codegen_push_var("n");
    codegen_push_int(1);
    codegen_binary_op(TOKEN_SUB, TYPE_INT);
    codegen_pop_to_var("a");

    //     val r1 = fib(a)
    codegen_def_var("r1");
    codegen_push_var("a");
    codegen_call_func("fib");
    codegen_pop_to_var("r1");

    //     val b = n - 2
    codegen_def_var("b");
    codegen_push_var("n");
    codegen_push_int(2);
    codegen_binary_op(TOKEN_SUB, TYPE_INT);
    codegen_pop_to_var("b");

    //     val r2 = fib(b)
    codegen_def_var("r2");
    codegen_push_var("b");
    codegen_call_func("fib");
    codegen_pop_to_var("r2");

    //     return r1 + r2
    codegen_push_var("r1");
    codegen_push_var("r2");
    codegen_binary_op(TOKEN_ADD, TYPE_INT);
    codegen_func_return();

    // end if
    codegen_if_end(if_id);
    codegen_func_end();

    // ==========================================
    // @main def runFib(): Unit =
    // ==========================================
    codegen_main_start();

    // val res = fib(10)
    codegen_def_var("res");
    codegen_push_int(10);
    codegen_call_func("fib");
    codegen_pop_to_var("res");

    // val s = toStr(res)
    codegen_def_var("s");
    codegen_push_var("res");
    codegen_builtin_to_str(TYPE_INT);
    codegen_pop_to_var("s");

    // val label = subStr("Fibonacci(10) = ???", 0, 16)
    codegen_def_var("label");
    codegen_push_string("Fibonacci(10) = ???");
    codegen_push_int(0);
    codegen_push_int(16);
    codegen_builtin_substr();
    codegen_pop_to_var("label");

    // print(label); print(s); print("\n")
    codegen_push_var("label");
    codegen_builtin_print(TYPE_STRING);

    codegen_push_var("s");
    codegen_builtin_print(TYPE_STRING);

    codegen_push_string("\n");
    codegen_builtin_print(TYPE_STRING);

    codegen_main_end();

    codegen_flush();
    codegen_free();
    return 0;
}
