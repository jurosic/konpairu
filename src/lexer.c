//lexer.c - Lexical analysis
//Project IFJ+IAL - 16.10.2026
//credit: Samuel Bončo


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "token.h"
#include "lexer.h"

//Safety limits for reading potentially infinite patterns
typedef enum {
    //APPLIED FOR ALL: (limits in characters, not counting \0)
    //identifier/keyword length limit 
    LEXERLIMIT_ID_LENGTH = 256,
    //num literal length limit, used for floats and ints
    LEXERLIMIT_NUMLIT_LENGTH = 256, 
    //string literal length limit
    LEXERLIMIT_STRLIT_LENGTH = 2048,
    //a reasonable indentation limit, 100 tabs worth of indentation
    LEXERLIMIT_INDENT_SIZE = 400,
} Limits;

// Keyword structure for elements in lookup table
typedef struct {
    const char* word;
    TokenType type;
} Keyword;

// Simple keyword lookup table :{word, TokenType}
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
    {"return", TOKEN_RETURN},
    {"true", TOKEN_LIT_BOOL},   //intentional duplicate,
    {"false", TOKEN_LIT_BOOL},  // bool value is identified by first letter
};


// Scan one character into the lexer and update lexer's position in the filestream.
// @return Scanned character. For convenience. (as type int for EOF)
int scan_char(Lexer *lexer) {
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
        scan_char(lexer);
}

// Reads and throws away the rest of a line comment
void read_line_comment(Lexer* lexer) {
    while (scan_char(lexer) != EOF && lexer->current_char != '\n') {}
    //EOF is captured by the main fetch function
}

// Reads and throws away the rest of a block comment (lexer still counts lines)
void read_block_comment(Lexer* lexer) {
    while (scan_char(lexer) != EOF) {
        if (lexer->current_char == '*' && scan_char(lexer) == '/') {
            //read next character to continue seamless reading
            scan_char(lexer); 
            return;
        }
    }
    //EOF is captured by the main fetch function
}

// Reads the indent count of the next non-empty line.
// @return TOKEN_NEWLINE with .indent_weight assigned.
// Returns TOKEN_INVALID if LEXERLIMIT_INDENT_SIZE reached.
Token read_indent(Lexer* lexer) {
    unsigned int indent_size = 0;
    // count the indent_weight by counting spaces from start of a new line
    // if a line is entirely empty, skip it and start counting from 0 again
    while (scan_char(lexer) == ' ' || lexer->current_char == '\n') {
        indent_size++;
        if (indent_size >= LEXERLIMIT_INDENT_SIZE) {
            lexer->err_msg = "Indentation limit exceeded.";
            lexer->column = 1; //point at start of indented line
            return create_token(TOKEN_INVALID);
        }
        if (lexer->current_char == '\n') indent_size = 0;
    }

    return create_token_newline(indent_size);
}

// Reads digits in integer, float or exponential form. Converts into token.
// Can stop reading at non-digit character, so math can be written like 1+2 instead of 1 + 2.
// @attention Numbers CAN be: 12 12_000  12.2  1.  1e2  1e-2  1e+2  1.5e-10  1e-003
// @attention Numbers CANNOT be: 012 1.2.2 1e0.5  1e.5  1e5.  
// @return TOKEN_LIT_INT with .int_val assigned or TOKEN_LIT_DOUBLE with .double_val assigned.
// Returns TOKEN_INVALID if the number is longer than LEXERLIMIT_NUMLIT_LENGTH or is in wrong format.
Token read_number(Lexer* lexer) {
    char scan_buffer[LEXERLIMIT_NUMLIT_LENGTH+1];
    scan_buffer[0] = lexer->current_char;
    int len = 1;
    int is_double = 0;
    int is_exponent_form = 0;
    int is_invalid = 0;
    int was_underscore = 0;

    while (isdigit(scan_char(lexer)) || strchr(".eE_-+", lexer->current_char)) {
        if (len >= LEXERLIMIT_NUMLIT_LENGTH) {
            lexer->err_msg = "Number literal length limit exceeded.";
            return create_token(TOKEN_INVALID);
        }

        //only one dot allowed, and has to be before 'e'
        if (lexer->current_char == '.') {
            if (is_double || is_exponent_form) {
                // found something like 123.2.105 or 12e0.01
                is_invalid = 1;
                break;
            }
            is_double = 1;
        }

        //only one 'e' allowed
        if (tolower(lexer->current_char) == 'e') {
            if (is_exponent_form) {
                // found two 'e's
                is_invalid = 1;
                break;
            }
            is_exponent_form = 1;
        }

        //sign only allowed right after 'e'
        //(sign anywhere else counts as operator between numbers)
        if (lexer->current_char == '+' || lexer->current_char == '-') {
            if (tolower(scan_buffer[len-1]) != 'e') {
                is_invalid = 1;
                break;
            }
        }

        //underscores can repeat and are only allowed to be between two digits
        if (!isdigit(lexer->current_char)) {
            if (lexer->current_char == '_') {
                // if the character before '_' isn't a digit, the literal is invalid
                if (!isdigit(scan_buffer[len-1])) {
                    is_invalid = 1;
                    break;
                }
                was_underscore = 1;
                continue; //skip writing into the scan_buffer, ATOI/ATOF don't accept '_'.
            // if current isn't a digit, and before is '_', the literal is invalid
            } else if (was_underscore) {
                is_invalid = 1;
                break;
            }
        }
        was_underscore = 0; 

        //write latest char, move on
        scan_buffer[len++] = lexer->current_char;
    }
    scan_buffer[len] = '\0';

    //number can't end in 'e' or '_'
    if (strchr("eE", scan_buffer[len-1]) && !was_underscore)
        is_invalid = 1;

    //return invalid if number is invalid
    if (is_invalid) {
        lexer->err_msg = "Invalid number literal.";
        return create_token(TOKEN_INVALID);
    }

    //if number has suffix 'd' or 'D', make sure it's gonne be type Double
    if (tolower(lexer->current_char) == 'd') {
        is_double = 1;
        scan_char(lexer); //prime lexer
    }

    //return int or double token after last check,
    // check if non-zero numbers dont start with zero (invalid literal)
    // ideally, i would check this before finishing scanning, but this is simpler
    if (is_double || is_exponent_form) {
        double val = atof(scan_buffer);
        if ((val != 0 && scan_buffer[0] != '0') || val == 0)
            return create_token_double(val);
    } else {
        int val = atoi(scan_buffer);
        if ((val != 0 && scan_buffer[0] != '0') || val == 0)
            return create_token_int(val);
    }

    //number is non-zero but starts with zero, literal is invalid
    lexer->err_msg = "Invalid number literal: Non-zero literal cannot begin with a zero.";
    return create_token(TOKEN_INVALID);
}

// Checks if word is keyword from the keywords lookup table.
// @returns TokenType of the found keyword or TOKEN_ID TokenType if word is not keyword.
// Returns TOKEN_LIT_BOOL for both "true" and "false".
TokenType find_keyword(char* word) {
    int num_keywords = sizeof(keywords) / sizeof(keywords[0]);
    
    for (int i = 0; i < num_keywords; i++)
        if (strcmp(word, keywords[i].word) == 0)
            return keywords[i].type; // found keyword
    
    // not a keyword, just a regular variable/function name
    return TOKEN_ID; 
}

// Reads word and checks if it's a keyword. If it isn't, it's an identifier.
// First letter has to be A-z or '_', the rest can be A-z or 0-9 or '-' or '_'.
// Single '_' counts as TOKEN_UNDERSCORE, not a name.
// ALSO checks for formatted string by seeing if the first letter is 's' followed by a string.
// @returns Any keyword token, TOKEN_ID with .name assigned, TOKEN_LIT_BOOL with .bool_val assigned,
// TOKEN_UNDERSCORE or TOKEN_INTERPOLATOR in case of single 's' followed by a string.
// Returns TOKEN_INVALID if LEXERLIMIT_ID_LENGTH reached.
Token read_keyword_or_id_or_interpolator(Lexer* lexer) {
    char scan_buffer[LEXERLIMIT_ID_LENGTH+1];
    scan_buffer[0] = lexer->current_char;
    int length = 1;

    //read text (symbols - _ seem to be allowed in names in documentation)
    while (isalnum(scan_char(lexer)) || lexer->current_char == '-' || lexer->current_char == '_') {
        if (length >= LEXERLIMIT_ID_LENGTH){
            lexer->err_msg = "Identifier length limit exceeded.";
            return create_token(TOKEN_INVALID);
        }
        scan_buffer[length++] = lexer->current_char;
    }
    scan_buffer[length] = '\0';

    //check for interpolated string
    if (length == 1 && scan_buffer[0] == 's' && lexer->current_char == '"')
        //found interpolated string and lexer is already primed to read string next fetch call
        return create_token(TOKEN_INTERPOLATOR);

    //check for single underscore
    if (length == 1 && scan_buffer[0] == '_')
        return create_token(TOKEN_UNDERSCORE);

    //check for keyword, return keyword token
    TokenType keyword_type = find_keyword(scan_buffer);
    if (keyword_type == TOKEN_ID)
        //found token, return TOKEN_ID with word as .name
        //STRDUP USES MALLOC!!! Warning about str/id token freeing is in token.h 
        return create_token_id(strdup(scan_buffer));

    //if "true" or "false" found, write as TOKEN_LIT_BOOL with .bool_val assigned.
    if (keyword_type == TOKEN_LIT_BOOL)
        return create_token_bool(scan_buffer[0] == 't');

    return create_token(keyword_type);
}

// Read stream until quotes, convert to token.
// Evaluates escaping characters too.
// Uses strdup to allocate final string from buffer.
// @return TOKEN_LIT_STR with .str_val assigned.
// Returns TOKEN_INVALID if string isn't ended or LEXERLIMIT_STRLIT_LENGTH reached.
Token read_string(Lexer* lexer) {
    char scan_buffer[LEXERLIMIT_STRLIT_LENGTH+1];
    int length = 0;
    int escaped = 0;

    while (length < LEXERLIMIT_STRLIT_LENGTH) {
        if (lexer->current_char == '"' && escaped == 0) {
            scan_char(lexer); //prime lexer
            break;
        }

        if (lexer->current_char == EOF || lexer->current_char == '\n') {
            // INVALID STRING, wasn't ended correctly
            lexer->err_msg = "Unfinished string literal.";
            return create_token(TOKEN_INVALID);
        }

        //handle escaping properly even though it only matters for \\ and \"
        escaped = (lexer->current_char == '\\' && escaped == 0);

        scan_buffer[length++] = lexer->current_char;
        scan_char(lexer);
    }
    scan_buffer[length] = '\0';

    return create_token_str(strdup(scan_buffer));
}

// Read stream until triple quotes, convert to token.
// As per documentation, multi-line strings don't evaluate escaping characters.
// Uses strdup to allocate final string from buffer.
// @return TOKEN_LIT_STR with .str_val assigned.
// Returns TOKEN_INVALID if string isn't ended or LEXERLIMIT_STRLIT_LENGTH reached.
Token read_multiline_string(Lexer* lexer) {
    char scan_buffer[LEXERLIMIT_STRLIT_LENGTH+1];
    scan_buffer[0] = scan_char(lexer);
    int length = 1;
    int quote_count = 0;

    while (length < LEXERLIMIT_STRLIT_LENGTH) {
        if (lexer->current_char == EOF) {
            // INVALID STRING, wasn't ended correctly
            lexer->err_msg = "Unfinished multiline string literal.";
            return create_token(TOKEN_INVALID);
        }

        if (lexer->current_char == '"') {
            quote_count++;
            if (quote_count == 3) {
                // Found the ending 3 quotes of the multiline string
                // backtrack buffer so '\0' ends string correctly.
                //buffer = [x,x,x,",",", , , ]
                //              ^--3---
                length = length-3;
                scan_char(lexer); //prime lexer
                break;
            }
        }else{
            quote_count = 0;
        }

        scan_buffer[length++] = lexer->current_char;
        scan_char(lexer);
    }
    scan_buffer[length] = '\0';

    return create_token_str(strdup(scan_buffer));
}

// Read one or a sequence of symbols and convert into token.
// @return Token corresponding to the found symbol/sequence.
// Returns TOKEN_INVALID if it's an unlisted symbol.
Token read_simple_symbols(Lexer *lexer, char prev_char) {
    switch (prev_char) {
        case '@': //@main
            if (lexer->current_char == 'm')
                if (scan_char(lexer) == 'a')
                    if (scan_char(lexer) == 'i')
                        if (scan_char(lexer) == 'n') {
                            //found legitimate @main token
                            scan_char(lexer); //prime next fetch
                            return create_token(TOKEN_MAINFUNC);
                        }
            // otherwise wrong use of @
            lexer->err_msg = "Invalid use of lexeme '@', can only be @main";
            return create_token(TOKEN_INVALID);
            
        case '+':
            return create_token(TOKEN_ADD);
        case '-':
            return create_token(TOKEN_SUB);
        case '*':
            return create_token(TOKEN_MUL);

        case '=':
            if (lexer->current_char == '=') {   // found ==
                scan_char(lexer); //prime lexer
                return create_token(TOKEN_EQUALS);
            }
            if (lexer->current_char == '>') {   // found =>
                scan_char(lexer); //prime lexer
                return create_token(TOKEN_ARROW_R);
            }
            return create_token(TOKEN_ASSIGN);  // found =

        case '!':
            if (lexer->current_char == '=') {   // found !=
                scan_char(lexer); //prime lexer
                return create_token(TOKEN_NOTEQUALS);
            }
            return create_token(TOKEN_NOT);     // found !

        case '<':
            if (lexer->current_char == '=') {   // found <=
                scan_char(lexer); //prime lexer
                return create_token(TOKEN_LESSEQUAL);
            }
            return create_token(TOKEN_LESS);    // found <

        case '>':
            if (lexer->current_char == '=') {   // found >=
                scan_char(lexer); //prime lexer
                return create_token(TOKEN_GREATEREQUAL);
            }
            return create_token(TOKEN_GREATER); // found >

        case '(':
            return create_token(TOKEN_PAREN_L);
        case ')':
            return create_token(TOKEN_PAREN_R);
        case '[':
            return create_token(TOKEN_BRACK_L);
        case ']':
            return create_token(TOKEN_BRACK_R);
      /*case '{':
            return create_token(TOKEN_BRACE_L);
        case '}':
            return create_token(TOKEN_BRACE_R);
      */
        case '|':
            if (lexer->current_char == '|') {   // found ||
                scan_char(lexer); //prime lexer
                return create_token(TOKEN_OR);
            }
            return create_token(TOKEN_UNION);    // found |

        case '&':
            if (lexer->current_char == '&') {   // found &&
                scan_char(lexer); //prime lexer
                return create_token(TOKEN_AND); 
            }
            //ERROR symbol just "&" isn't valid
            lexer->err_msg = "Invalid symbol '&'.";
            return create_token(TOKEN_INVALID);

        case ':':
            return create_token(TOKEN_COLON);
        case '.':
            return create_token(TOKEN_DOT);
        case ',':
            return create_token(TOKEN_COMMA);
        
        default:
            lexer->err_msg = "Invalid symbol.";
            return create_token(TOKEN_INVALID);
    }
}


// API: Processes and returns the next token from the stream
Token lexer_next_token(Lexer *lexer) {
    // Function expects that the next character for lexing is already primed.
    // Many sections require to read the next character before confirming, so
    //  instead of backtracking or "unscan_char()", it's done like this for simplicity.
    // tl;dr The next character for lexing isn't read now but before each fetch returns.

    // CHECK FOR WHITESPACE AND COMMENTS (also tokenize single '/')
    // logic is looped for cases like " /* */   //    \n  /**/ // "
    while (lexer->current_char == ' ' || lexer->current_char == '/') {
        //check if whitespace, pass whitespace
        read_whitespace(lexer);

        //check if comment, pass comment
        if (lexer->current_char == '/') {
            scan_char(lexer);
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
    // if()
    //      fisdjfisdif;        iffff


    //CHECK FOR NEWLINE, READ INDENT WEIGHT, RETURN TOKEN
    // also takes care of lines separated by ;
    if (lexer->current_char == '\n' || lexer->current_char == ';') {
        Token indent_token;
        static unsigned int last_indent_weight = 0;

        if (lexer->current_char == ';') {
            // in case of ';', eliminate whitespace first
            read_whitespace(lexer);
            // if space after ; is empty, ignore it (same as an empty line)
            if (lexer->current_char != '\n' && lexer->current_char != EOF) {
                // otherwise, create a newline token and continue next fetch as
                // if the next content was on a new line
                // (the indent_weight of this artificial new line is the same
                //  as the last indent weight)
                indent_token = create_token_newline(last_indent_weight);
            }
        } else {
            indent_token = read_indent(lexer);
            last_indent_weight = indent_token.value.indent_weight;
        }

        // read_indent throws away empty lines, so if after all those lines there is an EOF,
        // just forget the newline token
        // (remove the check for EOF if parser wants to check for an empty line at end of file)
        // (the '\n' check is for when a ';' is followed by emptiness until end of line)
        if (lexer->current_char != EOF && lexer->current_char != '\n')
            return indent_token;
    }

    //CHECK FOR EOF, RETURN TOKEN
    if (lexer->current_char == EOF)
        return create_token(TOKEN_EOF);
    
    //CHECK FOR NUMBER LITERAL, READ INT/FLOAT, RETURN TOKEN
    if (isdigit(lexer->current_char)) {
        return read_number(lexer);
    }

    //CHECK FOR ALPHA CHARACATER FOLLOWED BY ALPHANUM CHARACTERS
    // READ WORD, CHECK IF KEYWORD OR IDENTIFIER, RETURN TOKEN
    if (isalpha(lexer->current_char) || lexer->current_char == '_') {
        return read_keyword_or_id_or_interpolator(lexer);
    }

    //CHECK FOR QUOTES, READ AND SAVE STRING, RETURN TOKEN
    if (lexer->current_char == '"') {
        if (scan_char(lexer) == '"') {
            if (scan_char(lexer) == '"') {
                return read_multiline_string(lexer);
            }

            //found exactly 2 quotes, return empty string token
            scan_char(lexer); //prime lexer
            // Uses strdup even on an empty string for consistency, since every other instance of
            // a TOKEN_LIT_STR, created by this program, is also allocated and has to be freed.
            return create_token_str(strdup(""));
        }
        
        //found 1 quote, read string into token
        return read_string(lexer);
    }

    //LASTLY, CONVERT REST OF SYMBOLS TO TOKENS (except '/')
    //ALSO ChECKS FOR @main
    char prev_char = lexer->current_char;
    scan_char(lexer);   //prime lexer,this has to be before the switch statement
                        //(every symbol that needs to read symbols ahead is messy right now)
    return read_simple_symbols(lexer, prev_char);

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