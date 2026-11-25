#ifndef ERROR_CODES
#define ERROR_CODES

// Return error codes
#define LEXER_ERR_CODE 1            // Lexical error
#define SYNTAX_ERR_CODE 2           // Syntax error
#define SEM_ERR_CODE_NO_DEF 3       // Missing definition or no @main
#define SEM_ERR_CODE_REDEF 4        // Redefinition error
#define SEM_ERR_CODE_ARGS 5         // Function argument count/type mismatch
#define SEM_ERR_CODE_DEF_TYPE 6     // Type compatibility / inferrence error
#define SEM_ERR_CODE_ASSIGN 7       // Assignment to immutable constant
#define SEM_ERR_CODE_MISC 10        // Other semantic error (invalid return, etc.)
#define INTERNAL_ERR_CODE 99        // Compiler internal error (memory allocation, etc.)

#endif