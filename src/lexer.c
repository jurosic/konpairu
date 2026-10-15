
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "token.h"
#include "lexer.h"

//Safety limits for reading potentially infinite patterns
typedef enum {
    //APPLIED FOR ALL: (length limit in characters, not counting \0)
    //identificator/keyword length limit 
    LEXERLIMIT_ID_LENGTH = 256,
    //num literal length limit, used for floats and ints
    LEXERLIMIT_NUMLIT_LENGTH = 256, 
    //string literal length limit
    LEXERLIMIT_STRLIT_LENGTH = 2048,
} Limits;

// Keyword structure for use in searching keywords
typedef struct {
    const char* word;
    TokenType type;
} Keyword;

// List of all keywords
static const Keyword keywords[] = {
    {"import", TOKEN_IMPORT},
    {"val", TOKEN_VAL},
    {"var", TOKEN_VAR},
    {"class", TOKEN_CLASS},
    {"extension", TOKEN_EXTENSION},
    {"type", TOKEN_TYPE},
    {"Int", TOKEN_INT},
    {"Double", TOKEN_DOUBLE},
    {"String", TOKEN_STRING},
    {"Unit", TOKEN_UNIT},
    {"Boolean", TOKEN_BOOLEAN},
    {"List", TOKEN_LIST},
    {"if", TOKEN_IF},
    {"then", TOKEN_THEN},
    {"else", TOKEN_ELSE},
    {"case", TOKEN_CASE},
    {"match", TOKEN_MATCH},
    {"while", TOKEN_WHILE},
    {"do", TOKEN_DO},
    {"for", TOKEN_FOR},
    {"to", TOKEN_TO},
    {"until", TOKEN_UNTIL},
    {"by", TOKEN_BY},
    {"def", TOKEN_DEF},
    {"return", TOKEN_RETURN}
};

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
Token create_token_double(double token_val) {
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

// Reads and throws away whitespace
void read_whitespace(Lexer* lexer) {
    while (lexer->current_char == ' ')
        read_char(lexer);
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
            //read next character to continue seamless reading
            read_char(lexer); 
            return;
        }
    }
    //EOF is captured by the main fetch function
}

// Checks if word is keyword from keyword lookup table, returns TokenType.
// Returns TOKEN_ID TokenType if not keyword.
TokenType is_keyword(char* word) {
    int num_keywords = sizeof(keywords) / sizeof(keywords[0]);
    
    for (int i = 0; i < num_keywords; i++)
        if (strcmp(word, keywords[i].word) == 0)
            return keywords[i].type; // found keyword
    
    // not a keyword, just a regular variable/function name
    return TOKEN_ID; 
}




// API: Processes and returns the next token from the stream
Token lexer_next_token(Lexer *lexer) {

    // Check for whitespace and comments (also tokenize single '/')
    // logic is looped for cases like " /* */  /**/  "
    while (lexer->current_char == ' ' || lexer->current_char == '/') {
        //check if whitespace, pass whitespace
        read_whitespace(lexer);

        //check if comment, pass comment
        if (lexer->current_char == '/') {
            read_char(lexer);
            if (lexer->current_char == '/') {
                //line comment starting with //
                read_line_comment(lexer);
            } else if (lexer->current_char == '*') {
                //block comment starting with /*
                read_block_comment(lexer);
            } else {
                //if not a comment, just return a '/' token
                return create_token(TOKEN_DIV);
            }
        }
    }

    //check if new line, read indent weight, return token
    if (lexer->current_char == '\n') {
        unsigned int indent_size = 0;
        // count the indent_weight by counting spaces from start of a new line
        // if a line is entirely empty, skip it and start counting from 0 again
        while (read_char(lexer) == ' ' || lexer->current_char == '\n') {
            indent_size++;
            if (lexer->current_char == '\n') indent_size == 0;
        }
        // if after all those lines is an EOF, just forget the newline token
        // (remove this if parser wants to check for an empty new line at end of file)
        if (lexer->current_char != EOF)
            return create_token_newline(indent_size);
    }

    //check if EOF
    if (lexer->current_char == EOF)
        return create_token(TOKEN_EOF);
    
    //check if number literal, read int/float, return token
    if (isdigit(lexer->current_char)) {
        char scan_buffer[LEXERLIMIT_NUMLIT_LENGTH+1];
        scan_buffer[0] = lexer->current_char;
        int length = 1;
        int is_double = 0;

        while (length < LEXERLIMIT_NUMLIT_LENGTH && (isdigit(read_char(lexer)) || lexer->current_char == '.') ) {
            if (lexer->current_char == ".")
                is_double = 1;
            scan_buffer[length++] = lexer->current_char;
        }
        scan_buffer[length] = '\0';

        if (is_double) return create_token_double(atof(scan_buffer));
        else return create_token_int(atoi(scan_buffer));
    }

    //check if alpha character followed by alphanums, read text, check if keyword or ID, return token
    if (isalpha(lexer->current_char)) {
        char scan_buffer[LEXERLIMIT_ID_LENGTH+1];
        scan_buffer[0] = lexer->current_char;
        int length = 1;

        //read text
        while (length < LEXERLIMIT_ID_LENGTH && (isalnum(read_char(lexer)) || lexer->current_char == '-'))
            scan_buffer[length++] = lexer->current_char;
        scan_buffer[length] = '\0';

        //check for keyword, return keyword token
        TokenType keyword_type = is_keyword(scan_buffer);
        if (keyword_type != TOKEN_ID)
            return create_token(keyword_type);

        //return ID token with .name if not keyword
        //STRDUP USES MALLOC!!! Warning about str/id token freeing is in token.h 
        return create_token_id(strdup(scan_buffer));
    }

    //check if quotes, read and save string, return token
    if (lexer->current_char == '"') {
        if (read_char(lexer) == '"') {
            if (read_char(lexer) == '"') {
                //found 3 quotes, read multiline string into token
                char scan_buffer[LEXERLIMIT_STRLIT_LENGTH+1];
                scan_buffer[0] = read_char(lexer);
                int length = 1;
                int quote_count = 0;

                while (length < LEXERLIMIT_STRLIT_LENGTH) {
                    if (lexer->current_char != EOF) {
                        // INVALID STRING, wasn't ended correctly
                        // TODO: error reporting. Maybe returning a special error token and
                        //  putting an error message in the lexer struct?
                        length = 0; break; //currently returns empty string
                    }
                    if (lexer->current_char == '"') {
                        quote_count++;
                        if (quote_count == 3) {
                            // Found the ending 3 quotes of the multiline string
                            //buffer = [x,x,x,",",", , , ]
                            //              ^--3---
                            length = length-3;
                            break;
                        }
                    }else{
                        quote_count = 0;
                    }
                    scan_buffer[length++] = lexer->current_char;
                    read_char(lexer);
                }
                scan_buffer[length] = '\0';

                return create_token_id(strdup(scan_buffer));
            }

            //found exactly 2 quotes, return empty string token
            read_char(lexer); //prime lexer to read next char
            return create_token_str("");
        }
        
        //found 1 quote, read string into token
        char scan_buffer[LEXERLIMIT_STRLIT_LENGTH+1];
        int length = 0;

        while (lexer->current_char != '"' && length < LEXERLIMIT_STRLIT_LENGTH) {
            if (lexer->current_char == EOF || lexer->current_char == '\n') {
                // INVALID STRING, wasn't ended correctly
                // TODO: error reporting
                length = 0; break; //currently returns empty string
            }
            scan_buffer[length++] = lexer->current_char;
            read_char(lexer);
        }
        scan_buffer[length] = '\0';

        return create_token_id(strdup(scan_buffer));
    }

    //else convert non-alphanum symbols to tokens (except '/')
    //also checks for @main
    

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