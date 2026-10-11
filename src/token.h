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
    TOKEN_SPACE,        // single space
    TOKEN_TAB,          // tab of space
    TOKEN_NEWLINE,      // new line

    // Tokens with values (literals, identifiers)
    TOKEN_ID,           // any variable name - abc123
    TOKEN_LIT_INT,      // any integer  - 31
    TOKEN_LIT_DOUBLE,   // any float    - 3.1
    TOKEN_LIT_STR,      // any string   - "hi"

    // // KEYWORDS // //
    TOKEN_IMPORT,       // import
    TOKEN_VAL,          // val
    TOKEN_VAR,          // var
    TOKEN_CLASS,        // class    //EXTENSION
    TOKEN_EXTENSION,    // extension //EXTENSION
    TOKEN_TYPE,         // type     //EXTENSION
    // Datatypes
    TOKEN_INT,          // Int
    TOKEN_DOUBLE,       // Double
    TOKEN_STRING,       // String
    TOKEN_UNIT,         // Unit
    TOKEN_BOOLEAN,      // Boolean  //EXTENSION
    TOKEN_LIST,         // List     //EXTENSION
    // Logic
    TOKEN_IF,           // if
    TOKEN_THEN,         // then
    TOKEN_ELSE,         // else
    TOKEN_CASE,         // case     //EXTENSION
    TOKEN_MATCH,        // match    //EXTENSION
    // Loops
    TOKEN_WHILE,        // while
    TOKEN_DO,           // do
    TOKEN_FOR,          // for      //EXTENSION
    TOKEN_TO,           // to       //EXTENSION
    TOKEN_UNTIL,        // until    //EXTENSION
    TOKEN_BY,           // by    //EXTENSION
    // Functions
    TOKEN_DEF,          // def
    TOKEN_RETURN,       // return

    // Math operators
    TOKEN_PLUS,         // +
    TOKEN_MINUS,        // -
    TOKEN_DIVIDE,       // /
    TOKEN_TIMES,        // *

    // Relation operators
    TOKEN_EQUALS,       // ==
    TOKEN_NOT,          // !=
    TOKEN_LESS,         // <
    TOKEN_GREATER,      // >
    TOKEN_LESSEQUAL,    // <=
    TOKEN_GREATEREQUAL, // >=

    // Parentheses, Brackets, Braces
    TOKEN_PAREN_L,   // (
    TOKEN_PAREN_R,   // )
    TOKEN_BRACK_L,   // [
    TOKEN_BRACK_R,   // ]
    TOKEN_BRACE_L,   // {
    TOKEN_BRACE_R,   // }

    // idk
    TOKEN_ASSIGN,       // =
    TOKEN_COLON,        // :
    TOKEN_INTERPOLATOR, // s"Interpolated string ${1+1}"   //EXTENSION (only the s)
    
} TokenType;


#endif // TOKEN_H