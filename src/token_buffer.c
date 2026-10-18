#include "token_buffer.h"


#define EXPAND_BUFFER_MUL   2   //when full, multiply capacity
#define SHRINK_BUFFER_MUL   2   //when mostly empty, divide capacity

#define MIN_CAP_TO_SHRINK   8   //minimum capacity to shrink
#define MIN_SIZE_TO_SHRINK  2   //minimum size to shrink

#define SHRINK_CAP_TO_SIZE  4   //minimum ratio of size:cap to shrink



//frees value of token if needed
void free_token_content(Token* token){
    if(!token) return;
    
    if((token->type == TOKEN_LIT_STR || token->type == TOKEN_INTERPOLATOR) && token->value.str_val){
        free(token->value.str_val);
    }
    else if(token->type == TOKEN_ID && token->value.name){
        free(token->value.name);
    }
    //add more here if needed
}

//frees value of all tokens in buffer if needed
void free_all_token_content(TokenBuffer* buffer){

    for(size_t i = 0; i < buffer->size; i++){
        free_token_content(&(buffer->arr[i]));
    }
}

//frees buffer and returns NULL
void token_buffer_free(TokenBuffer* buffer){
    if(!buffer) return;
    
    free_all_token_content(buffer);
    free(buffer->arr);
    free(buffer);
    buffer = NULL;
}


//inits buffer to cap
//returns TokenBuffer pt if successful and cap > 0, NULL otherwise
TokenBuffer* token_buffer_init(size_t cap){
    if(cap == 0) return NULL;

    TokenBuffer* buffer = malloc(sizeof(TokenBuffer));
    if(!buffer) return NULL;

    buffer->arr = malloc(cap * sizeof(Token));
    if(!buffer->arr){
        free(buffer);
        return NULL;
    }

    buffer->cap = cap;
    buffer->size = 0;
    return buffer;
}

//capacity check and change
//returns as NULL if unsuccessful
void token_buffer_cap(TokenBuffer* buffer){
    if(!buffer) return;

    //expand arr if full
    if(buffer->cap == buffer->size){
        size_t new_cap = buffer->cap * EXPAND_BUFFER_MUL;
        Token* temp = realloc(buffer->arr, new_cap * sizeof(Token));

        if(!temp){
            token_buffer_free(buffer);
            return;
        }

        buffer->arr = temp;
        buffer->cap = new_cap;
        return;
    }

    //shrink arr mostly empty
    else if((buffer->size > MIN_SIZE_TO_SHRINK) &&
            (buffer->cap >= buffer->size * SHRINK_CAP_TO_SIZE) &&
            (buffer->cap > MIN_CAP_TO_SHRINK)){
        
        size_t new_cap = buffer->cap / SHRINK_BUFFER_MUL;
        Token* temp = realloc(buffer->arr, new_cap * sizeof(Token));
        
        if(!temp){
            token_buffer_free(buffer);
            return;
        }

        buffer->arr = temp;
        buffer->cap = new_cap;
        return;
    }
}


//loads Token from lexer to buffer at array[size]
//and calls token_buffer_cap
//return buffer as NULL if unsuccessful
void load_next_token(Lexer* Lexer, TokenBuffer* buffer){

    token_buffer_cap(buffer);
    if(!buffer) return;

    buffer->arr[buffer->size] = lexer_next_token(Lexer);
    buffer->size++;
}

//loads Token from lexer to buffer at array[0] and changes size to 1
//return buffer as NULL if unsuccessful
void load_first_token(Lexer* Lexer, TokenBuffer* buffer){
    if(!buffer) return;

    free_all_token_content(buffer);
    buffer->arr[0] = lexer_next_token(Lexer);
    buffer->size = 1;
    //buffer->last_idx = 0;
}

void set_first_token(TokenBuffer* buffer, Token token){
    if(!buffer) return;
    free_all_token_content(buffer);
    buffer->arr[0] = token;
    buffer->size = 1;
}

//TO DO: better name
//sets the last token as first, and changes size to 1
void token_buffer_set_as_first(TokenBuffer* buffer){
    if(!buffer || buffer->size <= 1) return;

    Token temp = buffer->arr[buffer->size-1];
   set_first_token(buffer, temp);
}