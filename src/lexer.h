#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "token.h"

// The lexer struct
typedef struct {
    FILE *stream;       // The input file stream being read character-by-character
    int current_char;   // The current character read from steam

    int line;           // Current line number for error reporting
    int column;         // Current column number for error reporting
    char* err_msg;      // Contains an error string after fetching any TOKEN_INVALID
} Lexer;


// Lexer "API" functions for the parser / syntax analyzer

// Initializes the lexer with a file stream
Lexer* lexer_init(FILE *stream);

// Processes and returns the next token from the stream
Token lexer_next_token(Lexer *lexer);

// Memory cleanup for the lexer object/struct
void lexer_free(Lexer *lexer);

#endif // LEXER_H
