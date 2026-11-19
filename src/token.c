//token.c - Token manipulation functions
//Project IFJ+IAL - 16.10.2026
//credit: Samuel Bončo


#include <stdlib.h>
#include "token.h"


Token create_token(TokenType token_t) {
    Token new_token;
    new_token.type = token_t;
    return new_token;
}

Token create_token_int(int token_val) {
    Token new_token = create_token(TOKEN_LIT_INT);
    new_token.value.int_val = token_val;
    return new_token;
}

Token create_token_bool(int token_val) {
    Token new_token = create_token(TOKEN_LIT_BOOL);
    new_token.value.bool_val = token_val;
    return new_token;
}

Token create_token_double(double token_val) {
    Token new_token = create_token(TOKEN_LIT_DOUBLE);
    new_token.value.double_val = token_val;
    return new_token;
}

Token create_token_str(char* token_strval) {
    Token new_token = create_token(TOKEN_LIT_STR);
    new_token.value.str_val = token_strval;
    return new_token;
}

Token create_token_id(char* token_name) {
    Token new_token = create_token(TOKEN_ID);
    new_token.value.name = token_name;
    return new_token;
}

Token create_token_newline(unsigned int indent_weight) {
    Token new_token = create_token(TOKEN_NEWLINE);
    new_token.value.indent_weight = indent_weight;
    return new_token;
}

void token_free(Token token) {
    if (token.type == TOKEN_ID)
        free(token.value.name);
        
    if (token.type == TOKEN_LIT_STR)
        free(token.value.str_val);
}
