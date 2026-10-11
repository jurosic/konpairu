#ifndef TOKEN_H
#define TOKEN_H


// Union for every possible datatype of token.value
typedef union {
    int int_val;            // For integer literals
    double float_val;       // For float literals
    char *str_val;          // For string literals
    //void *custom_val;     // Generic pointer for any custom value
} TokenValue;


// The core Token struct declaration
typedef struct {
    TokenType type;     // Token type
    TokenValue value;   // Extra metadata associated with the token
                        // (this is unused for all tokens except literals and names)
    
    // optional metadata for compile-time error messages
    //int line;           
    //int column;         
} Token;


// Enum of all possible token types
typedef enum {
    // File codes, white spaces
    TOKEN_EOF = 0,      // EOF
    
} TokenType;


#endif // TOKEN_H