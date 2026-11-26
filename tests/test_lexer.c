//text_lexer.c - Lexer unit testing
//Project IFJ+IAL - 16.11.2026
//credit: Samuel Bončo

#define _GNU_SOURCE //allows for standard posix functions like fmemopen, that aren't declared in c11

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
    token_free(t);
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

void test_numbers2(void) {
    Lexer *lexer = init_lexer_string("1e2-2  2e2+1");

    Token t;
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_DOUBLE);
    assert(t.value.double_val == 1e2);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_SUB);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_INT);
    assert(t.value.int_val == 2);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_DOUBLE);
    assert(t.value.double_val == 2e2);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_ADD);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_INT);
    assert(t.value.int_val == 1);

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


    //TEST WINDOWS 
    free_lexer_string(lexer);
    
    printf("PASSED: test_indents\n");
}

void test_windows_newlines(void) {
    Token t;
    Lexer *lexer = init_lexer_string(
        "\r\n"
        "def\r\n"
        "var \r\n"
        "val"
    );
    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_DEF);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_VAR);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_VAL);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_EOF);

    printf("PASSED: test_windows_newlines\n");
}

void test_strings(void) {
    Token t;
    Lexer *lexer = init_lexer_string(
        "\"\"  "        //empty string
        "\"normalstring\"  "
        "\"\"\"\"\"\""  //empty multistring
        "\"\"\"multiline\nstring\"\"\"  "
        "s\"nicestring\"  "
        "s\"\"\"multinice\"\"\""
    );

    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_STR);
    assert(strcmp(t.value.str_val, "") == 0);
    token_free(t);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_STR);
    assert(strcmp(t.value.str_val, "normalstring") == 0);
    token_free(t);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_STR);
    assert(strcmp(t.value.str_val, "") == 0);
    token_free(t);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_STR);
    assert(strcmp(t.value.str_val, "multiline\nstring") == 0);
    token_free(t);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_INTERPOLATOR);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_STR);
    assert(strcmp(t.value.str_val, "nicestring") == 0);
    token_free(t);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_INTERPOLATOR);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_LIT_STR);
    assert(strcmp(t.value.str_val, "multinice") == 0);
    token_free(t);
    
    free_lexer_string(lexer);
    printf("PASSED: test_strings\n");
}

void test_comments(void) {
    Token t;
    Lexer *lexer = init_lexer_string(
        "def //thiscomment \n"
        "//thiscomment var \n"
        "if /* block comment */ then \n"
        "/**/while/***/do // \n"
        "def/* * //* * \n"
        "return */*/ \n"
        "/*  do for def var \n"
        " var def for do  "
    );

    t = lexer_next_token(lexer); assert(t.type == TOKEN_DEF);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);

    //t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_IF);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_THEN);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);

    t = lexer_next_token(lexer); printf(" %i ", t.type);
    assert(t.type == TOKEN_WHILE);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_DO);
    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_DEF);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_NEWLINE);

    t = lexer_next_token(lexer); assert(t.type == TOKEN_EOF);

    free_lexer_string(lexer);
    printf("PASSED: test_comments\n");
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
    Token t = lexer_next_token(lexer); assert(t.type == TOKEN_INVALID);
    assert(lexer->err_msg != NULL);
    free_lexer_string(lexer);

    lexer = init_lexer_string("\"\"\"hello world");
    t = lexer_next_token(lexer); assert(t.type == TOKEN_INVALID);
    assert(lexer->err_msg != NULL);
    free_lexer_string(lexer);

    lexer = init_lexer_string("\"\"\"hello world\"\"");
    t = lexer_next_token(lexer); assert(t.type == TOKEN_INVALID);
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
    // Generate an id name longer than the limit
    char long_id[LEXERLIMIT_ID_LENGTH+100];
    memset(long_id, 'a', LEXERLIMIT_ID_LENGTH+99);
    long_id[LEXERLIMIT_ID_LENGTH+99] = '\0';

    Lexer *lexer = init_lexer_string(long_id);
    Token t = lexer_next_token(lexer);
    
    assert(t.type == TOKEN_INVALID);
    assert(lexer->err_msg != NULL);

    free_lexer_string(lexer);
    printf("PASSED: test_id_length_exceeded\n");
}

void test_number_literal_length_exceeded(void) {
    // Generate a number longer than the limit
    char long_id[LEXERLIMIT_NUMLIT_LENGTH+100];
    memset(long_id, '1', LEXERLIMIT_NUMLIT_LENGTH+99);
    long_id[LEXERLIMIT_NUMLIT_LENGTH+99] = '\0';

    Lexer *lexer = init_lexer_string(long_id);
    Token t = lexer_next_token(lexer);
    
    assert(t.type == TOKEN_INVALID);
    assert(lexer->err_msg != NULL);

    free_lexer_string(lexer);
    printf("PASSED: test_number_literal_length_exceeded\n");
}

void test_string_literal_length_exceeded(void) {
    int limit = LEXERLIMIT_STRLIT_LENGTH+100;

    // -- Single line string test
    char *long_str = malloc(limit + 3);
    assert(long_str != NULL);

    long_str[0] = '"';
    for (int i = 1; i <= limit; i++) {
        long_str[i] = 'a';
    }
    long_str[limit + 1] = '"';
    long_str[limit + 2] = '\0';

    Lexer *lexer1 = init_lexer_string(long_str);
    Token t1 = lexer_next_token(lexer1);
    assert(t1.type == TOKEN_INVALID);
    token_free(t1);
    
    free_lexer_string(lexer1);
    free(long_str);

    // -- Multiline string test
    char *long_mstr = malloc(limit + 7); //6 for quotes, 1 for \0
    assert(long_mstr != NULL);

    strcpy(long_mstr, "\"\"\"");
    for (int i = 3; i < limit + 3; i++) long_mstr[i] = 'b';
    strcpy(long_mstr + limit + 3, "\"\"\"");

    Lexer *lexer2 = init_lexer_string(long_mstr);
    Token t2 = lexer_next_token(lexer2);
    assert(t2.type == TOKEN_INVALID);
    token_free(t2);

    free_lexer_string(lexer2);
    free(long_mstr);

    printf("PASSED: test_string_literal_length_exceeded\n");
}

void test_indent_size_exceeded(void) {
    int limit = LEXERLIMIT_INDENT_SIZE+50;

    // allocate: '\n' + spaces + "val" + '\0'
    char *input = malloc(limit + 5);
    assert(input != NULL);

    input[0] = '\n';
    for (int i = 1; i <= limit; i++) input[i] = ' ';
    strcpy(input + limit + 1, "val");

    Lexer *lexer = init_lexer_string(input);
    Token t = lexer_next_token(lexer);
    
    // first token should be indent thats too long
    assert(t.type == TOKEN_INVALID);
    assert(lexer->err_msg != NULL);

    free_lexer_string(lexer);
    free(input);

    printf("PASSED: test_indent_size_exceeded\n");
}


// --- MAIN ---
int main(void) {
    printf("=== LEXER UNIT TESTS ===\n");
    
    test_keywords_and_identifiers();
    test_main_annotation();
    test_numbers();
    test_numbers2();
    test_indents();
    test_windows_newlines();
    test_strings();
    test_comments();
    
    test_invalid_numbers();
    test_unfinished_string();
    test_invalid_symbols();
    test_invalid_at_symbol();
    test_id_length_exceeded();
    test_number_literal_length_exceeded();
    test_string_literal_length_exceeded();
    test_indent_size_exceeded();

    printf("=== LEXER UNIT TESTS DONE ===\n");
    return 0;
}