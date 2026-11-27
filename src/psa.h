#ifndef PSA_H
#define PSA_H

#include "lexer.h"
#include "token.h"
#include "semantics.h"
#include "codegen.h"



//'<'   =   shift
//'>'   =   reduction
//'='   =   equal
//' '   =   error
static const char prec_table[7][7] = {
    //*/     +-     REL         (        )        i        END
    {'>',    '>',     '>',     '<',     '>',     '<',     '>'},   //*/
    {'<',    '>',     '>',     '<',     '>',     '<',     '>'},   //*-
    {'<',    '<',     ' ',     '<',     '>',     '<',     '>'},   //REL
    {'<',    '<',     '<',     '<',     '=',     '<',     ' '},   //(
    {'>',    '>',     '>',     ' ',     '>',     ' ',     '>'},   //)
    {'>',    '>',     '>',     ' ',     '>',     ' ',     '>'},   //i
    {'<',    '<',     '<',     '<',     '$',     '<',     '$'}   //END
};


//index for operators in table
typedef enum{
    PT_MUL_DIV = 0,             //*, /
    PT_ADD_SUB,                 //+, -
    PT_REL,                     //<, <=, >, >=, ==, !=
    PT_LPAREN,                  //(
    PT_RPAREN,                  //)
    PT_OPERAND,                 //ID, LIT_INT, LIT_DOUBLE, LIT_STR, LIT_BOOL
    PT_END,                     //END (end of expression)
    PT_INVALID
}PrecTableIndex;

//type of item
typedef enum{
    PREC_SYM_TERMINAL = 0,      //terminal token
    PREC_SYM_NONTERM,           // nonterminal
    PREC_SYM_SHIFT              //'<' start of reduction
}PrecSymType;

//item of PrecStack
typedef struct{
    PrecSymType sym_type;
    TokenType token_type;
    DataType data_type;
    char name[LEXERLIMIT_ID_LENGTH + 1];
    TokenValue value;
}PrecStackItem;


typedef struct{
    PrecStackItem *items;
    size_t cap;
    size_t top;
}PrecStack;

PrecStack* prec_stack_init(void);

void prec_stack_free(PrecStack *s);

//pushes item to the top of stack
//expands cap if needed
bool prec_stack_push(PrecStack *s, PrecStackItem item);

//TOKEN into PT
PrecTableIndex token_to_pt_index(TokenType t);

//returns topmost terminal from stack
PrecTableIndex prec_stack_top_terminal_index(PrecStack *s);

//inserts PREC_SHIFT right after topmost terminal
bool prec_stack_insert_shift(PrecStack *s);

int reduce_rule(PrecStack *s);

#endif