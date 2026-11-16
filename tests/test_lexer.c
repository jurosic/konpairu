//text_lexer.c - Lexer unit testing
//Project IFJ+IAL - 16.11.2026
//credit: Samuel Bončo

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lexer.h"
#include "token.h"

// helper function to make lexer from string
static Lexer* init_lexer_string(const char* str) {
    FILE *stream = fmemopen((void*)str, strlen(str), "r");
    assert(stream != NULL);
    return lexer_init(stream);
}

// helper function to free lexer created from string
static void free_lexer_string(Lexer *lexer) {
    if (lexer) {
        if (lexer->stream) fclose(lexer->stream);
        lexer_free(lexer);
    }
}

// --- TESTS ---
void test_keywords_and_identifiers(void) {
    Lexer *lexer = init_lexer_string("val var if then else def my_var-name _ __");
    //doesn't include EVERY keyword cuz im too lazy and its the same checker,
    //so if a few work then probably all will work
    Token t;
    t = lexer_next_token(lexer); assert(t.type == TOKEN_VAL);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_VAR);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_IF);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_THEN);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_ELSE);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_DEF);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_ID);
    assert(strcmp(t.value.name, "my_var-name") == 0);
    token_free(t);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_UNDERSCORE);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_ID);
    assert(strcmp(t.value.name, "__") == 0);
    token_free(t);

    free_lexer_string(lexer);
    printf("PASSED: test_keywords_and_identifiers\n");
}

void test_main_annotation(void) {
    Lexer *lexer = init_lexer_string("@main def foo(): Unit = ()");
    
    Token t;
    t = lexer_next_token(lexer); assert(t.type == TOKEN_MAINFUNC);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_DEF);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_ID);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_PAREN_L);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_PAREN_R);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_COLON);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_UNIT);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_ASSIGN);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_PAREN_L);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_PAREN_R);

    free_lexer_string(lexer);
    printf("PASSED: test_main_annotation\n");
}

void test_numbers(void) {
    Lexer *lexer = init_lexer_string("12  12_000  1____0  12.2  1.  1e2  1e-2  1e+2  1.5e-10  1e-003");

    Token t;
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_INT);
    assert(t.value.int_val == 12);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_INT);
    assert(t.value.int_val == 12000);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_INT);
    assert(t.value.int_val == 10);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_DOUBLE);
    assert(t.value.double_val == 12.2);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_DOUBLE);
    assert(t.value.double_val == 1.0);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_DOUBLE);
    assert(t.value.double_val == 1e2);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_DOUBLE);
    assert(t.value.double_val == 1e-2);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_DOUBLE);
    assert(t.value.double_val == 1e2);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_DOUBLE);
    assert(t.value.double_val == 1.5e-10);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_DOUBLE);
    assert(t.value.double_val == 1e-003);

    free_lexer_string(lexer);
    printf("PASSED: test_numbers\n");
}

void test_indents(void) {
    Token t;
    Lexer *lexer = init_lexer_string(
        "   \n"
        "def\n"
        "   val\n"
        "   var; var\n"
        "   if;;   ;;;   \n    \n \n  ;  \n"
        "      def ;def; def \n"
        ";;;; val"                  //should have an indent of 1
    );

    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);
    assert(t.value.indent_weight == 0);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_DEF);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);
    assert(t.value.indent_weight == 3);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_VAL);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);
    assert(t.value.indent_weight == 3);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_VAR);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);
    assert(t.value.indent_weight == 3);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_VAR);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);
    assert(t.value.indent_weight == 3);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_IF);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);
    assert(t.value.indent_weight == 6);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_DEF);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);
    assert(t.value.indent_weight == 6);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_DEF);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);
    assert(t.value.indent_weight == 6);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_DEF);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);
    assert(t.value.indent_weight == 1);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_VAL);
    
    t = lexer_next_token(lexer); assert(t.type == TOKEN_EOF);

    free_lexer_string(lexer);
    printf("PASSED: test_indents\n");
}

// --- ERROR PATH TESTS ---
void test_invalid_numbers(void) {
    Lexer* lexer; Token t;

    lexer = init_lexer_string("012");
    t = lexer_next_token(lexer); assert(t.type == TOKEN_INVALID);
    assert(lexer->err_msg != NULL);
    free_lexer_string(lexer);

    lexer = init_lexer_string("1.2.2");
    t = lexer_next_token(lexer); assert(t.type == TOKEN_INVALID);
    assert(lexer->err_msg != NULL);
    free_lexer_string(lexer);

    lexer = init_lexer_string("1e0.5");
    t = lexer_next_token(lexer); assert(t.type == TOKEN_INVALID);
    assert(lexer->err_msg != NULL);
    free_lexer_string(lexer);

    lexer = init_lexer_string("1e.5");
    t = lexer_next_token(lexer); assert(t.type == TOKEN_INVALID);
    assert(lexer->err_msg != NULL);
    free_lexer_string(lexer);

    lexer = init_lexer_string("1e5.");
    t = lexer_next_token(lexer); assert(t.type == TOKEN_INVALID);
    assert(lexer->err_msg != NULL);
    free_lexer_string(lexer);

    printf("PASSED: test_invalid_numbers\n");
}

void test_unfinished_string(void) {
    Lexer *lexer = init_lexer_string("\"hello world");

    Token t = lexer_next_token(lexer);
    assert(t.type == TOKEN_INVALID);
    assert(lexer->err_msg != NULL);

    free_lexer_string(lexer);
    printf("PASSED: test_unfinished_string\n");
}

void test_invalid_symbols(void) {
    Lexer *lexer = init_lexer_string("&");

    Token t = lexer_next_token(lexer);
    assert(t.type == TOKEN_INVALID);
    assert(lexer->err_msg != NULL);

    free_lexer_string(lexer);
    printf("PASSED: test_invalid_symbols\n");
}

void test_invalid_at_symbol(void) {
    // Znak '@' iný ako @main
    Lexer *lexer = init_lexer_string("@other");

    Token t = lexer_next_token(lexer);
    assert(t.type == TOKEN_INVALID);

    free_lexer_string(lexer);
    printf("PASSED: test_invalid_at_symbol\n");
}

void test_id_length_exceeded(void) {
    // Generate an id name longer than the limit (256 chars)
    char long_id[300];
    memset(long_id, 'a', 299);
    long_id[299] = '\0';

    Lexer *lexer = init_lexer_string(long_id);
    Token t = lexer_next_token(lexer);
    
    assert(t.type == TOKEN_INVALID);
    assert(lexer->err_msg != NULL);

    free_lexer_string(lexer);
    printf("PASSED: test_id_length_exceeded\n");
}


// --- MAIN ---
int main(void) {
    printf("=== LEXER UNIT TESTS ===\n");
    
    test_keywords_and_identifiers();
    test_main_annotation();
    test_numbers();
    test_indents(); //unfinished
    
    test_invalid_numbers();
    test_unfinished_string();
    test_invalid_symbols();
    test_invalid_at_symbol();
    test_id_length_exceeded();

    printf("=== LEXER UNIT TESTS DONE ===\n");
    return 0;
}