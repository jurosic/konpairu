#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "token.h"

// The lexer struct
typedef struct {
    FILE* stream;       // The input filestream being read character-by-character
    int current_char;   // The current character read from steam

    int line;           // Current line number for error reporting
    int column;         // Current column number for error reporting
    const char* err_msg;      // Contains an error string after fetching any TOKEN_INVALID
                        //  Will never be dynamically allocated, no need to free().
} Lexer;


// Lexer "API" functions for the parser / syntax analyzer

// Initializes the lexer with a file stream
// @return Lexer pointer. Returns NULL if file stream is invalid or memory allocation failed.
Lexer* lexer_init(FILE *stream);

// Fetches the next token from the lexers stream
// @return Token. Returns Token of TokenType TOKEN_INVALID if lexer encounters unexpected lexeme.
// Whenever a TOKEN_INVALID is returned, the .err_msg variable of lexer is assigned a non-empty string
// describing the invalid lexeme.
Token lexer_next_token(Lexer *lexer);

// Memory cleanup for the lexer object/struct
// @warning DOES NOT fclose() the filestream. Only the lexer struct is freed.
void lexer_free(Lexer *lexer);

#endif // LEXER_H
