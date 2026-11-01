#include "bst.h"
#include "token.h"
#include <stdbool.h>

// Initializes the semantic module (creates the global scope tree).
void sem_init();

// Called by the parser every time it encounters 'if', 'while', or a new function.
// Pushes a new empty AVL tree onto the scope stack.
void sem_enter_scope();

// Called by the parser at the end of a block.
// Destroys the top AVL tree (discarding local variables) and pops the stack.
void sem_leave_scope();

// Destroys the global scope and cleans up everything at the end of compilation.
void sem_dispose();

// Called when parser reads: val x: Int = 5 (or var x = 5)
// Checks if 'key' already exists in the CURRENT scope (Error 4).
// If not, inserts it into the top AVL tree.
int sem_define_variable(char *key, DataType type, bool is_mutable);

// Called when a variable is used in an expression (e.g., y = x + 2).
// Searches from the local scope up to the global scope.
// Returns Error 3 if not found. If found, writes its type into *out_type.
int sem_get_variable_type(char *key, DataType *out_type);

// Called when parser reads: x = 10
// 1. Checks if 'x' exists (Error 3).
// 2. Checks if 'x' is a 'val' (Error 7 - cannot modify constant).
// 3. Checks if 'expr_type' matches the variable's type (Error 6 - type mismatch).
int sem_check_assignment(char *key, DataType expr_type);

// Called by precedence analysis when reducing an expression like (expr1 + expr2).
// Checks if operator is valid for type1 and type2.
// Handles IFJ26 implicit conversion (Int -> Double).
// Returns Error 6 if incompatible (e.g., Int + String).
// If successful, writes the resulting type into *out_result_type.
int sem_eval_binary_op(DataType type1, DataType type2, TokenType operator, DataType *out_result_type);

// (Optional) Same as above, but for unary operators like -x or !x
int sem_eval_unary_op(DataType type, TokenType operator, DataType *out_result_type);

// Called when parser reads: def foo(x: Int): String
// Checks for redefinition (Error 4).
// Inserts function signature (key, param count, param types, return type) into the GLOBAL tree.
int sem_define_function(char *key, DataType return_type, int param_count, DataType *param_types);

// Called when parser reads: foo(10, "hello")
// 1. Checks if function exists (Error 3).
// 2. Checks if param_count matches (Error 5).
// 3. Checks if arg_types match the defined param types (Error 5).
// If successful, writes the function's return type into *out_return_type.
int sem_check_function_call(char *key, int arg_count, DataType *arg_types, DataType *out_return_type);

// Called when parser reads: return expr
// Compares 'expr_type' with the expected return type of the CURRENTLY parsed function.
// Returns Error 6 on type mismatch or if return is used where it shouldn't be.
int sem_check_return(DataType current_func_expected_type, DataType expr_type);