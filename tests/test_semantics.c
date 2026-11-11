#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h" // Your semantics header
#include "bst.h"       // Your BST header
#include "token.h"

// A simple macro for testing. If the condition fails, it prints an error.
#define TEST_ASSERT(test_name, condition) \
    do { \
        if (condition) { \
            printf("[OK]   %s\n", test_name); \
        } else { \
            printf("[FAIL] %s\n", test_name); \
            printf("       Line: %d\n", __LINE__); \
        } \
    } while(0)


void test_scopes_and_variables() {
    printf("--- Testing Scopes and Variables ---\n");
    
    // 1. Initialization
    sem_init();
    
    // 2. Simulation: entered a function body (first local scope)
    sem_enter_scope();
    
    // Add variable 'x' (e.g., var x: Int)
    SymbolData var_x;
    var_x.symbol_type = SYMBOL_VAR;
    var_x.type = TYPE_INT;
    
    int err = sem_define_variable("x", var_x, true);
    TEST_ASSERT("Definition of new variable 'x' is successful", err == 0);
    
    // Try adding 'x' again in the same block (Error 4)
    err = sem_define_variable("x", var_x, true);
    TEST_ASSERT("Redefinition of 'x' in the same block returns error 4", err == 4);
    
    // 3. Simulation: entered an IF block (new nested scope)
    sem_enter_scope();
    
    // Definition of 'x' in the new block must succeed (shadowing)
    var_x.type = TYPE_STRING; // New 'x' is a string
    err = sem_define_variable("x", var_x, true);
    TEST_ASSERT("Shadowing variable 'x' in a new nested block is successful", err == 0);
    
    // Simulation: looking up 'x'. Semantics should find the most local one (STRING)
    DataType found_val;
    DataType *found = &found_val;
    sem_get_variable_type("x", found);
    TEST_ASSERT("Found shadowed 'x' is of type String", found != NULL && *found == TYPE_STRING);
    
    // 4. Simulation: left the IF block
    sem_leave_scope();
    
    // Looking up 'x' again. Now we should find the original one (INT)
    sem_get_variable_type("x", found);
    TEST_ASSERT("After leaving the block, 'x' returns to original type Int", found != NULL && *found == TYPE_INT);
    
    // 5. Cleanup at the end of the program
    sem_dispose();
}


void test_binary_operations() {
    printf("\n--- Testing Binary Operations ---\n");
    
    bool convert_left = false, convert_right = false;
    DataType res_type;
    int err;

    // Test 1: Int + Int = Int (no conversion)
    err = sem_eval_binary_op(TYPE_INT, TYPE_INT, &convert_left, &convert_right, TOKEN_ADD, &res_type);
    TEST_ASSERT("Int + Int is OK", err == 0);
    TEST_ASSERT("Result is Int", res_type == TYPE_INT);
    TEST_ASSERT("No conversion needed", convert_left == false && convert_right == false);

    // Test 2: Int + Double = Double (left operand conversion)
    err = sem_eval_binary_op(TYPE_INT, TYPE_DOUBLE, &convert_left, &convert_right, TOKEN_ADD, &res_type);
    TEST_ASSERT("Int + Double is OK", err == 0);
    TEST_ASSERT("Result is Double", res_type == TYPE_DOUBLE);
    TEST_ASSERT("Left operand is converted", convert_left == 1 && convert_right == 0);

    // Test 3: String + Int = Error 6
    err = sem_eval_binary_op(TYPE_STRING, TYPE_INT, &convert_left, &convert_right, TOKEN_ADD, &res_type);
    TEST_ASSERT("String + Int returns error 6 (type incompatibility)", err == 6);

    // Test 4: String == String -> Boolean (no conversion)
    err = sem_eval_binary_op(TYPE_STRING, TYPE_STRING, &convert_left, &convert_right, TOKEN_EQUALS, &res_type);
    TEST_ASSERT("String == String is OK", err == 0);
    TEST_ASSERT("Result is Boolean", res_type == TYPE_BOOLEAN);
    
    // Test 5: Double < Int = Boolean (right operand conversion)
    err = sem_eval_binary_op(TYPE_DOUBLE, TYPE_INT, &convert_left, &convert_right, TOKEN_LESS, &res_type);
    TEST_ASSERT("Double < Int is OK", err == 0);
    TEST_ASSERT("Comparison result is Boolean", res_type == TYPE_BOOLEAN);
    TEST_ASSERT("Right operand is converted to Double", convert_left == 0 && convert_right == 1);
}


void test_functions() {
    printf("\n--- Testing Functions ---\n");
    
    sem_init();
    
    DataType params[2] = {TYPE_INT, TYPE_DOUBLE};
    
    // 1. Correct definition of a global function
    int err = sem_define_function("my_func", TYPE_STRING, 2, params);
    TEST_ASSERT("Adding global function is successful", err == 0);
    
    // 2. Duplicity check
    err = sem_define_function("my_func", TYPE_INT, 0, NULL);
    TEST_ASSERT("Redefining function returns error 4", err == 4);
    
    // 3. Attempting to add a function in a local scope (parser fallback protection)
    sem_enter_scope(); // Entered a local block
    err = sem_define_function("nested_func", TYPE_UNIT, 0, NULL);
    TEST_ASSERT("Nested function definition returns internal error (99 or 9)", err == 99 || err == 9);
    sem_leave_scope();
    sem_leave_scope(); // Leave empty scope
    sem_dispose();
}


int main() {
    printf("Running Unit Tests for IFJ26 Semantics...\n\n");
    
    test_scopes_and_variables();
    test_binary_operations();
    test_functions();
    
    printf("\nTests finished.\n");
    return 0;
}