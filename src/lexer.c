

#include <stdlib.h>
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

// Create token of type TOKEN_LIT_STR with value token_strval
Token create_token_str(char* token_strval) {
    Token new_token = create_token(TOKEN_LIT_STR);
    new_token.value.str_val = token_strval;
    return new_token;
}

// Create token of type TOKEN_ID with value token_name
Token create_token_id(char* token_name) {
    Token new_token = create_token(TOKEN_ID);
    new_token.value.name = token_name;
    return new_token;
}

// Create token of type TOKEN_NEWLINE with value indent_weight
Token create_token_newline(unsigned int indent_weight) {
    Token new_token = create_token(TOKEN_NEWLINE);
    new_token.value.indent_weight = indent_weight;
    return new_token;
}


// Read and return one chracater
int read_char(Lexer *lexer) {
    // Update lexer attributes based on current char
    if (lexer->current_char == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else if (lexer->current_char != EOF) {
        lexer->column++;
    }

    // Fetch new char
    lexer->current_char = fgetc(lexer->stream);
    return lexer->current_char;
}

// Reads and throws away the rest of a line comment
void read_line_comment(Lexer* lexer) {
    while (read_char(lexer) != EOF && lexer->current_char != '\n') {}
    //EOF is captured by the main fetch function
}
// Reads and throws away the rest of a block comment (still counts lines)
void read_block_comment(Lexer* lexer) {
    while (read_char(lexer) != EOF) {
        if (lexer->current_char == '*' && read_char(lexer) == "/") {
            //read next character to pass entire comment block
            read_char(lexer); 
            return;
        }
    }
    //EOF is captured by the main fetch function
}

// API: Processes and returns the next token from the stream
Token lexer_next_token(Lexer *lexer) {
    //check if whitespace, pass whitespace
    while (lexer->current_char == ' ')
        read_char(lexer);

    //check if comment, pass comment (and tokenize single /)
    if (lexer->current_char == '/') {
        read_char(lexer);
        if (lexer->current_char == '/') {
            //line comment starting with //
            read_line_comment(lexer);
        } else if (lexer->current_char == '*') {
            //block comment starting with /*
            read_block_comment(lexer);
        } else {
            return create_token(TOKEN_DIV);
        }
    }

    //check if EOF
    
    //check if new line, read indent weight, return token
    
    //check if number, read int/float, return token

    //check if letter followed by alphanum, read name, check if keyword, return token

    //check if quotes, read and save string, return token

    //else convert non-alphanum symbols to tokens (except /)

}


// API: Initializes the lexer with a file stream
Lexer* lexer_init(FILE *stream) {
    //check if stream is valid
    if (stream == NULL) return NULL;

    //i am sorry for deciding to use pointers and malloc for the lexer
    //it is completely unnecessary
    Lexer* lexer = malloc(sizeof(Lexer));
    if (lexer == NULL) return NULL;

    lexer->stream = stream;
    lexer->current_char = fgetc(stream); // setup first character
    lexer->line = 1;
    lexer->column = 1;

    return lexer;
}

// API: Memory cleanup for the lexer object/struct
void lexer_free(Lexer *lexer) {
    free(lexer); //😭
}