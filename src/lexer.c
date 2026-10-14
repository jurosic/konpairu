


#include <stdio.h>
#include "token.h"
#include "lexer.h"

// Create empty token of type token_t
Token create_token(TokenType token_t);
// Create token of type TOKEN_LIT_INT with value token_val
Token create_token_int(int token_val);
// Create token of type TOKEN_LIT_DOUBLE with value token_val
Token create_token_float(double token_val);
// Create token of type TOKEN_LIT_STR with value token_val
Token create_token_str(char* token_strval);
// Create token of type TOKEN_ID with value token_val
Token create_token_id(char* token_idname);
// Create token of type TOKEN_NEWLINE with value token_val
Token create_token_newline(unsigned int indent_weight);


// Initializes the lexer with a file stream
Lexer* lexer_init(FILE *stream);

// Processes and returns the next token from the stream
Token lexer_next_token(Lexer *lexer);

// Memory cleanup for the lexer object/struct
void lexer_free(Lexer *lexer);
