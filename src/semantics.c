#include "semantics.h"
#include "bst.h"
#include "token.h"
#include <stdlib.h>
#include <stdbool.h>

// A single frame on the scope stack
typedef struct Scope {
    int scope_id;           // Specific id for scope
    bst_node_t *tree;       // Root of this scope's AVL tree
    struct Scope *outer;    // Pointer to the enclosing (parent) scope
} Scope;

// File-scoped state (invisible outside semantics.c)
static Scope *current_scope = NULL;       // Points to the topmost (most local) scope
static bst_node_t *global_symtable = NULL; // Dedicated tree for global identifiers (functions)

// Incremented counter for assigning ids to scopes
static int next_scope_id = 1;

bool is_numeric(DataType type){
    if(type == TYPE_INT || type == TYPE_DOUBLE) return true;
    return false;
}

// Initializes the semantic module (creates the global scope tree).
void sem_init(){
    bst_init(&global_symtable);
    current_scope = NULL;
    next_scope_id = 1; // Reset for clean unit testing
}

// Called by the parser every time it encounters 'if', 'while', or a new function.
// Pushes a new empty AVL tree onto the scope stack.
int sem_enter_scope(){
    Scope *new_scope = malloc(sizeof(Scope));
    if(new_scope == NULL) return 99;

    bst_init(&(new_scope->tree));
    new_scope->scope_id = next_scope_id++;
    new_scope->outer = current_scope;
    current_scope = new_scope;

    return 0;
}

// Called by the parser at the end of a block.
// Destroys the top AVL tree (discarding local variables) and pops the stack.
void sem_leave_scope(){
    if(current_scope == NULL) return;

    Scope *to_destroy = current_scope;
    current_scope = current_scope->outer;

    bst_destroy(&(to_destroy->tree));
    free(to_destroy);
}

// Destroys the global scope and cleans up everything at the end of compilation.
void sem_dispose(){
    while (current_scope != NULL) sem_leave_scope();
    
    bst_destroy(&global_symtable);
}

SymbolData* sem_get_variable(char *key){
    Scope *scope = current_scope;

    while (scope != NULL) {
        SymbolData *val = bst_search(scope->tree, key);
        if (val != NULL) {
            return val; // Found in the closest enclosing scope
        }
        scope = scope->outer;
    }

    return NULL;
}

// Called when parser reads: val x: Int = 5 (or var x = 5)
// Checks if 'key' already exists in the CURRENT scope (Error 4).
// If not, inserts it into the top AVL tree.
int sem_define_variable(char *key, SymbolData val, bool is_mutable){
    val.symbol_type = SYMBOL_VAR;
    val.data.var_data.is_mutable = is_mutable;

    return bst_insert(&(current_scope->tree), key, val);
}

// Called when a variable is used in an expression (e.g., y = x + 2).
// Searches from the local scope up to the global scope.
// Returns Error 3 if not found. If found, writes its type into *out_type.
int sem_get_variable_type(char *key, DataType *out_type){
    SymbolData *val = sem_get_variable(key);

    if(val != NULL) {
        *out_type = val->type;
        return 0;
    }
    return 3;
    // !not accounting for global scope
}

// Called when parser reads: x = 10
// 1. Checks if 'x' exists (Error 3).
// 2. Checks if 'x' is a 'val' (Error 7 - cannot modify constant).
// 3. Checks if 'expr_type' matches the variable's type (Error 6 - type mismatch).
int sem_check_assignment(char *key, DataType expr_type){
    SymbolData *val = sem_get_variable(key);

    if(val == NULL) return 3;
    if(!val->data.var_data.is_mutable) return 7;
    if(val->type != expr_type) return 6;

    return 0;
}

// Called by precedence analysis when reducing an expression like (expr1 + expr2).
// Checks if operator is valid for type1 and type2.
// Handles IFJ26 implicit conversion (Int -> Double).
// Returns Error 6 if incompatible (e.g., Int + String).
// If successful, writes the resulting type into *out_result_type.
int sem_eval_binary_op(DataType type1, DataType type2, bool *convert_left, bool *convert_right, TokenType operator, DataType *out_result_type){
    if (convert_left) *convert_left = false;
    if (convert_right) *convert_right = false;

    switch(operator){
        case TOKEN_ADD:
            if(type1 == TYPE_STRING && type2 == TYPE_STRING){
                *out_result_type = TYPE_STRING;
                return 0;
            }
            // fallthrough
        case TOKEN_SUB: // fallthrough
        case TOKEN_MUL: // fallthough
        case TOKEN_DIV:
            if(is_numeric(type1) && is_numeric(type2)){
                *out_result_type = TYPE_INT;

                if(type1 == TYPE_DOUBLE || type2 == TYPE_DOUBLE){
                    *out_result_type = TYPE_DOUBLE;
                    // Conversion check
                    if (type1 == TYPE_INT && convert_left) *convert_left = true;
                    if (type2 == TYPE_INT && convert_right) *convert_right = true;
                }

                return 0;
            }
            return 6;
        case TOKEN_EQUALS: // fallthrough
        case TOKEN_NOTEQUALS: // fallthough
            if((type1 == TYPE_UNIT && type2 == TYPE_UNIT)  || (type1 == TYPE_BOOLEAN && type2 == TYPE_BOOLEAN)){
                *out_result_type = TYPE_BOOLEAN;
                return 0;
            }
            /* fallthrough */
        case TOKEN_LESS:        // fallthrough
        case TOKEN_GREATER:     // fallthrough
        case TOKEN_LESSEQUAL:   // fallthrough
        case TOKEN_GREATEREQUAL:
            if((type1 == TYPE_STRING && type2 == TYPE_STRING) || (is_numeric(type1) && is_numeric(type2))){
                *out_result_type = TYPE_BOOLEAN;

                // Conversion check
                if(type1 == TYPE_DOUBLE || type2 == TYPE_DOUBLE) {
                    if (type1 == TYPE_INT && convert_left) *convert_left = true;
                    if (type2 == TYPE_INT && convert_right) *convert_right = true;
                }

                return 0;
            }
            return 6;
        default:
            return 99;
    }
}

// (Optional) Same as above, but for unary operators like -x or !x
int sem_eval_unary_op(DataType type, TokenType operator, DataType *out_result_type){
    if(operator == TOKEN_SUB){
        if(is_numeric(type)){
            *out_result_type = type;
            return 0;
        }
        return 6;
    }

    return 99;
}

// Called when parser reads: def foo(x: Int): String
// Checks for redefinition (Error 4).
// Inserts function signature (key, param count, param types, return type) into the GLOBAL tree.
int sem_define_function(char *key, DataType return_type, int param_count, DataType *param_types){
    if(current_scope != NULL) return 99; // Error, functions should be defined only in global scope(parser should catch)

    // Move data to one variable
    SymbolData symbol_data;
    symbol_data.symbol_type = SYMBOL_FUNC;
    symbol_data.type = return_type;
    symbol_data.data.func_data.param_count = param_count;
    symbol_data.data.func_data.param_types = param_types;
    
    return bst_insert(&(global_symtable), key, symbol_data);
}

// Called when parser reads: foo(10, "hello")
// 1. Checks if function exists (Error 3).
// 2. Checks if param_count matches (Error 5).
// 3. Checks if arg_types match the defined param types (Error 5).
// If successful, writes the function's return type into *out_return_type.
int sem_check_function_call(char *key, int arg_count, DataType *arg_types, DataType *out_return_type){
    SymbolData *func_data = bst_search(global_symtable, key);

    if(func_data == NULL) return 3;
    if(func_data->data.func_data.param_count != arg_count) return 5;
    for(int i = 0; i < func_data->data.func_data.param_count; i++){
        if(func_data->data.func_data.param_types[i] != arg_types[i]) return 5;
    }

    *out_return_type = func_data->type;
    return 0;
}

// Called when parser reads: return expr
// Compares 'expr_type' with the expected return type of the CURRENTLY parsed function.
// Returns Error 6 on type mismatch or if return is used where it shouldn't be.
int sem_check_return(DataType current_func_expected_type, DataType expr_type){
    if(current_func_expected_type == expr_type) return 0;

    else return 6;
}