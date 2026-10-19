#ifndef TOKEN_BUFFER
#define TOKEN_BUFFER


#include "token.h"
#include "lexer.h"


//buffer for tokens
typedef struct{
    size_t cap;         //capacity of arr
    size_t size;        //num of tokens
    Token* arr;         //dynamic array of tokens
}TokenBuffer;



//frees value of token if needed
void free_token_content(Token* token);

//frees value of all tokens in buffer if needed
void free_all_token_content(TokenBuffer* buffer);

//frees buffer memory
//returns as NULL
void token_buffer_free(TokenBuffer* buffer);


//init the buffer to the cap
//returns TokenBuffer pt if successful and cap > 0, NULL otherwise
TokenBuffer* token_buffer_init(size_t cap);

//changes buffer capacity
//doubles buffer size: if its full
//halfes buffer size:  if its 1/4 full
//                     and its capacity is more than 8
//                     and its size is more than 2
//returns as NULL if unsuccessful
void token_buffer_cap(TokenBuffer* buffer);    


//loads Token from lexer to buffer at array[size]
//and calls token_buffer_cap
//return buffer as NULL if unsuccessful
void load_next_token(Lexer* lexer, TokenBuffer* buffer);

//loads Token from lexer to buffer at array[0] and changes size to 1
//return buffer as NULL if unsuccessful
void load_first_token(Lexer* lexer, TokenBuffer* buffer);

//sets token as the first token in buffer and changes size to 1
void set_first_token(TokenBuffer* buffer, Token token);

//TO DO: better name
//sets the last token as first, and changes size to 1
void token_buffer_set_as_first(TokenBuffer* buffer);


#endif