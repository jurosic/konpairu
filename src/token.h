#ifndef TOKEN_H
#define TOKEN_H


// Union for every possible datatype of token.value
typedef union {
    int int_val;            // For integer literals
    int bool_val;           // For boolean literals
    double double_val;      // For float literals
    char *str_val;          // For string literals
    char *name;             // For identificator names
    unsigned int indent_weight; // For new lines
} TokenValue;
//WARNING!!
//TOKENS USING .str_val OR .name NEED TO BE FREED USING free() PROPERLY
//free(token.str_val) / free(token.name)


// The core Token struct declaration
typedef struct {
    TokenType type;     // Token type
    TokenValue value;   // Extra metadata associated with the token
                        // (this is unused for all tokens except literals, names and indents)
    
    // optional metadata for compile-time error messages
    //int line;           
    //int column;         
} Token;


// Enum of all possible token types
typedef enum {
    // File reading
    TOKEN_EOF = 0,      // EOF
    //TOKEN_SPACE,      // single space
    //TOKEN_TAB,        // tab of space

    // Tokens with values (newline, identifiers, literals)
    TOKEN_NEWLINE,      // new line     - .indent_weight = int 0..n
    TOKEN_ID,           // any named thing - .name = abc123
    TOKEN_LIT_INT,      // any integer  - .int_val = 31
    TOKEN_LIT_BOOL,     // true/false   - .bool_val = int 1/0
    TOKEN_LIT_DOUBLE,   // any float    - .double_val = 3.1
    TOKEN_LIT_STR,      // any string   - .str_val = "hi"

    // // KEYWORDS // //
    TOKEN_IMPORT,       // import
    TOKEN_VAL,          // val
    TOKEN_VAR,          // var
    TOKEN_CLASS,        // class     //EXTENSION ALGEBDT (no idea what it means)
    TOKEN_TYPE,         // type      //EXTENSION ALGEBDT
    TOKEN_EXTENSION,    // extension //EXTENSION EXTKW
    // Datatypes
    TOKEN_INT,          // Int
    TOKEN_DOUBLE,       // Double
    TOKEN_STRING,       // String
    TOKEN_UNIT,         // Unit
    TOKEN_BOOLEAN,      // Boolean  //EXTENSION BOOLTHEN
    TOKEN_LIST,         // List     //EXTENSION LISTS
    // Logic
    TOKEN_IF,           // if
    TOKEN_THEN,         // then
    TOKEN_ELSE,         // else
    TOKEN_CASE,         // case     //EXTENSION ALGEBDT
    TOKEN_MATCH,        // match    //EXTENSION ALGEBDT
    TOKEN_UNDERSCORE,   // _        //EXTENSION ALGEBDT (used in match case default)
    // Loops
    TOKEN_WHILE,        // while
    TOKEN_DO,           // do
    TOKEN_FOR,          // for      //EXTENSION CONTROL
    TOKEN_TO,           // to       //EXTENSION CONTROL
    TOKEN_UNTIL,        // until    //EXTENSION CONTROL
    TOKEN_BY,           // by       //EXTENSION CONTROL
    // Functions
    TOKEN_DEF,          // def
    TOKEN_RETURN,       // return

    // Math operators
    TOKEN_ADD,          // +
    TOKEN_SUB,          // -
    TOKEN_DIV,          // /
    TOKEN_MUL,          // *

    // Relation operators
    TOKEN_EQUALS,       // ==
    TOKEN_NOTEQUALS,    // !=
    TOKEN_LESS,         // <
    TOKEN_GREATER,      // >
    TOKEN_LESSEQUAL,    // <=
    TOKEN_GREATEREQUAL, // >=

    // Parentheses, Brackets, Braces
    TOKEN_PAREN_L,      // (
    TOKEN_PAREN_R,      // )
    TOKEN_BRACK_L,      // [        //EXTENSION LISTS
    TOKEN_BRACK_R,      // ]        //EXTENSION LISTS
    //TOKEN_BRACE_L,    // {        // isn't used, only INSIDE formatted strings
    //TOKEN_BRACE_R,    // }        // -||-

    // Boolean algebra EXTENSION BOOLTHEN
    TOKEN_NOT,          // !
    TOKEN_OR,           // ||
    TOKEN_AND,          // &&
    TOKEN_TRUE,         // true
    TOKEN_FALSE,        // false

    // idk
    TOKEN_MAINFUNC,     // @main
    TOKEN_ASSIGN,       // =
    TOKEN_COLON,        // :
    TOKEN_DOT,          // .
    TOKEN_COMMA,        // ,
    TOKEN_ARROW_R,      // =>       //EXTENSION ALGEBDT and LAMBDAS
    TOKEN_UNION,        // |        //EXTENSION ALGEBDT
    TOKEN_INTERPOLATOR, // s"Interpolated string ${1+1}"    //EXTENSION INTERPOLATEDS
                        // (only the s is changed into this token)
                        // (the rest is still a string literal)
    
    TOKEN_INVALID,      // Used to report lexeme error to parser. Has no value, but
                        // lexer object gains .error_message whenever this token is returned.
    
} TokenType;


#endif // TOKEN_H
