#ifndef TOKEN_H
#define TOKEN_H


// Union for every possible datatype of token.value
typedef union {
    int int_val;            // For integer literals
    int bool_val;           // For boolean literals
    double double_val;      // For float literals
    char *str_val;          // For string literals
    char *name;             // For identifier names
    unsigned int indent_weight; // For new lines
} TokenValue;

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
    TOKEN_UNDERSCORE,   // _        //EXTENSION ALGEBDT (used in match case default)
    TOKEN_ARROW_R,      // =>       //EXTENSION ALGEBDT and LAMBDAS
    TOKEN_UNION,        // |        //EXTENSION ALGEBDT
    TOKEN_INTERPOLATOR, // s"Interpolated string ${1+1}"    //EXTENSION INTERPOLATEDS
                        // (only the s is changed into this token)
                        // (the rest is still a string literal)
    
    TOKEN_INVALID,      // Used to report lexeme error to parser. Has no value, but
                        // lexer object gains .err_msg whenever this token is returned.
    
} TokenType;

// The core Token struct
// @warning EVERY TOKEN SHOULD BE FREED USING token_free().
// This is because every token of type TOKEN_ID or TOKEN_LIT_STR has malloc'd values.
// The rest of the tokens do not have any allocated values, they can (but shouldn't) be ignored.
typedef struct {
    TokenType type;     // Token type
    TokenValue value;   // Extra metadata associated with the token
                        // (this is unused for all tokens except literals, names and indents)
    
    // optional metadata for compile-time error messages
    //int line;           
    //int column;         
} Token;


// Returns empty token of TokenType token_t
Token create_token(TokenType token_t);

// Returns token of TokenType TOKEN_LIT_INT with value token_val
Token create_token_int(int token_val);

// Returns token of TokenType TOKEN_LIT_BOOL with value .bool_val = token_val
// Function doesn't check if token_val is in range <0,1>.
Token create_token_bool(int token_val);

// Returns token of TokenType TOKEN_LIT_DOUBLE with value .double_val = token_val
Token create_token_double(double token_val);

// Returns token of TokenType TOKEN_LIT_STR with value .str_val = token_strval
Token create_token_str(char* token_strval);

// Returns token of TokenType TOKEN_ID with value .name = token_name
Token create_token_id(char* token_name);

// Returns token of TokenType TOKEN_NEWLINE with value .indent_weight = indent_weight
Token create_token_newline(unsigned int indent_weight);

// Free any token from memory, if it has a freeable TokenValue.
// Does nothing for every token that isn't of TokenType TOKEN_ID or TOKEN_LIT_STR.
void token_free(Token token);


#endif // TOKEN_H
