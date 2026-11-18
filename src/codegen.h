#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>
#include "bst.h"
#include "token.h"

// Initializes the code internal buffer and emits the .IFJcode26 header.
// Returns 0 on success, 99 on malloc failure.
int codegen_init(void);

// Frees all internal memory used by codegen
void codegen_free(void);

// Outputs filled code buffer to stdout
void codegen_flush(void);

// Emits program start to code buffer
void codegen_program_start(void);

// --- Variables & Literals ---
// Declares a variable in the current local frame (DEFVAR LF@name)
void codegen_def_var(const char *var_name);

// Pops the top of the data stack into a local variable (POPS LF@name)
void codegen_pop_to_var(const char *var_name);

// Pushes a local variable's value onto the data stack (PUSHS LF@name)
void codegen_push_var(const char *var_name);

// Pushes literals onto the data stack
void codegen_push_int(int val);
void codegen_push_double(double val);
void codegen_push_string(const char *str);
void codegen_push_unit(void);


// --- Expressions & Conversions ---
// Converts the top of the stack from Int to Float (INT2FLOATS)
void codegen_int2float_top(void);

// Converts the second item from the top of the stack from Int to Float
void codegen_int2float_second(void);

// Emits a unary operation on the top of the stack (e.g. TOKEN_SUB for unary minus)
void codegen_unary_op(TokenType op);

// Emits a binary operation on the top two stack items.
// result_type is needed to distinguish Int division (IDIVS) from Double division (DIVS),
// and String concatenation (CONCAT) from numeric addition (ADDS).
void codegen_binary_op(TokenType op, DataType operand_type);


// --- Control Flow (if / else & while) ---
// Returns a unique integer ID for pairing labels in nested if/while statements.
int codegen_next_label_id(void);

// Call right after the 'if' condition expression is evaluated (bool is on stack).
// Jumps to the 'else' label if the condition is false.
void codegen_if_start(int label_id);

// Call when transitioning from the 'then' block to the 'else' block.
void codegen_if_else(int label_id);

// Call at the very end of the 'if-else' statement.
void codegen_if_end(int label_id);

// Call BEFORE evaluating the 'while' condition expression (places the loop start label).
void codegen_while_header(int label_id);

// Call right AFTER evaluating the 'while' condition expression (bool is on stack).
// Jumps to loop end if the condition is false.
void codegen_while_body_start(int label_id);

// Call at the end of the 'while' body (jumps back to header and places end label).
void codegen_while_end(int label_id);

// --- Functions and @main ---
// Starts buffering declarations and instructions for the @main entry function.
// Returns 0 on success, 99 on malloc failure.
int codegen_main_start(void);

// Finalizes the @main function (stitches hoisted DEFVARs + body and emits EXIT int@0).
void codegen_main_end(void);

// Starts buffering declarations and instructions for a user function 'def func_name(...)'.
// Returns 0 on success, 99 on malloc failure.
int codegen_func_start(const char *func_name);

// Finalizes a user function (stitches hoisted DEFVARs + body and emits default Unit RETURN).
void codegen_func_end(void);

// Declares a formal parameter in LF and pops its argument from the data stack.
// NOTE: Because arguments are pushed left-to-right, pop parameters in REVERSE order.
void codegen_func_param(const char *param_name);

// Emits a call to a user-defined function (assumes arguments are already pushed onto stack).
// Leaves the function's return value on top of the data stack.
void codegen_call_func(const char *func_name);

// Emits an explicit return from a user function (assumes return value is on stack).
void codegen_func_return(void);

// Discards the top value of the data stack (used when ignoring a standalone call's return value).
void codegen_discard_stack_top(void);

// --- Built-in Functions ---
// Pops the top value from the stack and prints it (Unit prints as "()").
void codegen_builtin_print(DataType arg_type);

// Reads a line from stdin and pushes the resulting String onto the stack.
void codegen_builtin_read_line(void);

// Reads an Int from stdin (pushes 0 on EOF or invalid input).
void codegen_builtin_read_int(void);

// Reads a Double from stdin (pushes 0.0 on EOF or invalid input).
void codegen_builtin_read_double(void);

// Converts the top value on the stack to String (Double formatted as %.2f, Unit as "()").
void codegen_builtin_to_str(DataType arg_type);

// Truncates a Double on top of the stack to Int (FLOAT2INTS).
void codegen_builtin_to_int(void);

// Pops a String from the stack and pushes its character length (Int).
void codegen_builtin_str_length(void);

// Pops an Int ASCII code from the stack and pushes a 1-character String.
void codegen_builtin_chr(void);

// Pops (s: String, idx: Int) from the stack and pushes the ASCII code (or 0 if out of bounds).
void codegen_builtin_ord(void);

// Pops (s1: String, s2: String) from the stack and pushes -1, 0, or 1.
void codegen_builtin_strcmp(void);

// Pops (s: String, from: Int, to: Int) from the stack and pushes substring [from, to).
void codegen_builtin_substr(void);

#endif // CODEGEN_H
