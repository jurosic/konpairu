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
// Call when entering the @main function.
int codegen_main_start(void);

// Call when leaving the @main function.
void codegen_main_end(void);

// Call when entering a regular user function 'def name(...)'.
int codegen_func_start(const char *func_name);

// Call when leaving a regular user function.
void codegen_func_end(void);

#endif // CODEGEN_H
