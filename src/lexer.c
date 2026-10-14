


#include <stdio.h>
#include "token.h"
#include "lexer.h"


// Create empty token of type token_t
Token create_token(TokenType token_t) {
    Token new_token;
    new_token.type = token_t;
    return new_token;
}

// Create token of type TOKEN_LIT_INT with value token_val
Token create_token_int(int token_val) {
    Token new_token = create_token(TOKEN_LIT_INT);
    new_token.value.int_val = token_val;
    return new_token;
}

// Create token of type TOKEN_LIT_DOUBLE with value token_val
Token create_token_float(double token_val) {
    Token new_token = create_token(TOKEN_LIT_DOUBLE);
    new_token.value.int_val = token_val;
    return new_token;
}

// Create token of type TOKEN_LIT_STR with value token_val
Token create_token_str(char* token_strval) {
    Token new_token = create_token(TOKEN_LIT_STR);
    new_token.value.str_val = token_strval;
    return new_token;
}

// Create token of type TOKEN_ID with value token_val
Token create_token_id(char* token_name) {
    Token new_token = create_token(TOKEN_ID);
    new_token.value.name = token_name;
    return new_token;
}

// Create token of type TOKEN_NEWLINE with value token_val
Token create_token_newline(unsigned int indent_weight) {
    Token new_token = create_token(TOKEN_NEWLINE);
    new_token.value.indent_weight = indent_weight;
    return new_token;
}


// Initializes the lexer with a file stream
Lexer* lexer_init(FILE *stream);

// Processes and returns the next token from the stream
Token lexer_next_token(Lexer *lexer);

// Memory cleanup for the lexer object/struct
void lexer_free(Lexer *lexer);
