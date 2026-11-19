#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


#include "psa.h"
#include "semantics.h"



#define INIT_STACK_CAP 16
#define STACK_CAP_MUL 2         //multiplier for stack capacity if full



PrecStack* prec_stack_init(void){
    PrecStack *s = malloc(sizeof(PrecStack));
    if(!s) return NULL;

    s->items = malloc(INIT_STACK_CAP * sizeof(PrecStackItem));
    if(!s->items){
        free(s);
        return NULL;
    }
    s->top = 0;
    s->cap = INIT_STACK_CAP;
    PrecStackItem end_item = {   .sym_type = PREC_SYM_TERMINAL,
                        .token_type = TOKEN_EOF,
                        .data_type = TYPE_UNIT,
                        .name = NULL};
    
    s->items[s->top++] = end_item;
    return s;
}

void prec_stack_free(PrecStack *s){
    if(s){
        free(s->items);
        free(s);
    }
}


//pushes item to the top of stack
//expands cap if needed
bool prec_stack_push(PrecStack *s, PrecStackItem item){
    if(!s)  return false;

    if(s->top >= s->cap){
        size_t new_cap = s->cap * STACK_CAP_MUL;
        PrecStackItem *new_items = realloc(s->items, new_cap * sizeof(PrecStackItem));

        if(!new_items)  return false;
        s->items = new_items;
        s->cap = new_cap;
    }

    s->items[s->top++] = item;
    return true;
}

//TOKEN into PT
PrecTableIndex token_to_pt_index(TokenType t){
    switch (t){
        case TOKEN_MUL:
        case TOKEN_DIV:
            return PT_MUL_DIV;

        case TOKEN_ADD:
        case TOKEN_SUB:
            return PT_ADD_SUB;

        case TOKEN_LESS:
        case TOKEN_LESSEQUAL:
        case TOKEN_GREATER:
        case TOKEN_GREATEREQUAL:
        case TOKEN_EQUALS:
        case TOKEN_NOTEQUALS:
            return PT_REL;

        case TOKEN_PAREN_L:
            return PT_LPAREN;

        case TOKEN_PAREN_R:
            return PT_RPAREN;

        case TOKEN_ID:
        case TOKEN_LIT_INT:
        case TOKEN_LIT_DOUBLE:
        case TOKEN_LIT_STR:
        case TOKEN_LIT_BOOL:
            return PT_OPERAND;

        default:
            return PT_END;
    }
}

//returns topmost terminal from stack
PrecTableIndex prec_stack_top_terminal_index(PrecStack *s){
    if(!s) return PT_END;

    for(size_t i = s->top ; i > 0; i--){
        if(s->items[i - 1].sym_type == PREC_SYM_TERMINAL){
            return token_to_pt_index(s->items[i - 1].token_type);
        }
    }
    return PT_END;
}

//TO DO: use size_t insted of int
//inserts PREC_SHIFT right after topmost terminal
bool prec_stack_insert_shift(PrecStack *s){
    if(!s)  return false;

    int target_idx = - 1;
    for(int i = (int)s->top - 1; i >= 0; i--){
        if(s->items[i].sym_type == PREC_SYM_TERMINAL){
            target_idx = i;
            break;
        }
    }
    
    if(target_idx == -1)    return false;
    target_idx++;

    //cap check and expand
    if(s->top >= s->cap){
        size_t new_cap = s->cap * STACK_CAP_MUL;
        PrecStackItem *new_items = realloc(s->items, new_cap * sizeof(PrecStackItem));
        if(!new_items)      return false;

        s->items = new_items;
        s->cap = new_cap;
    }    

    //shift item by 1 to the right
    for(size_t i = s->top; i > target_idx; i--){
        s->items[i] = s->items[i - 1];
    }

    //asign PREC_SYM_SHIFT
    s->items[target_idx].sym_type = PREC_SYM_SHIFT;
    s->items[target_idx].token_type = TOKEN_EOF;
    s->items[target_idx].data_type = TYPE_UNIT;
    s->items[target_idx].name = NULL;
    s->top++;

    return true;
}

int reduce_rule(PrecStack *s){
    int mark_idx = (int) s->top -1;
    while(mark_idx >= 0 && s->items[mark_idx].sym_type != PREC_SYM_SHIFT){
        mark_idx--;
    }

    if(mark_idx < 0){
        return 2;
    }

    int handle_len = (int) s->top -1 -mark_idx;
    PrecStackItem *handle = &s->items[mark_idx + 1];

    PrecStackItem result_nonterm;
    result_nonterm.sym_type = PREC_SYM_NONTERM;
    result_nonterm.token_type = TOKEN_EOF;
    result_nonterm.name = NULL;

    //<E> -> i
    if(handle_len == 1 && handle[0].sym_type == PREC_SYM_TERMINAL){
        
        switch(handle->token_type){
            case TOKEN_LIT_INT:
                result_nonterm.data_type = TYPE_INT;
                codegen_push_int(handle[0].value.int_val);
                break;
            case TOKEN_LIT_DOUBLE:
                result_nonterm.data_type = TYPE_DOUBLE;
                codegen_push_double(handle[0].value.double_val);
                break;
            case TOKEN_LIT_STR:
                result_nonterm.data_type = TYPE_STRING;
                codegen_push_string(handle[0].value.str_val);
                break;
            case TOKEN_LIT_BOOL:
                result_nonterm.data_type = TYPE_BOOLEAN;
                codegen_push_int(handle[0].value.bool_val);
                break;
            case TOKEN_ID:
                DataType id_type;
                int sem_res = sem_get_variable_type(handle->name, &id_type);
                if(sem_res != 0)    return sem_res;
                result_nonterm.data_type = id_type;
                //TO DO: semantics func for scope?
                //temporary scope_id = 0 
                int scope_id = 0;
                codegen_push_var(handle[0].name, scope_id);

                break;
            default:
                return 2;
            
        }
    }

    //<E> -> ( <E> )
    else if(handle_len == 3 &&
            handle[0].sym_type == PREC_SYM_TERMINAL &&
            handle[0].token_type == TOKEN_PAREN_L &&
            handle[1].sym_type == PREC_SYM_NONTERM &&
            handle[2].sym_type == PREC_SYM_TERMINAL &&
            handle[2].token_type == TOKEN_PAREN_R){

        result_nonterm.data_type = handle[1].data_type;
    }

    //<E> -> <E> op <E>
    else if(handle_len == 3 &&
            handle[0].sym_type == PREC_SYM_NONTERM &&
            handle[1].sym_type == PREC_SYM_TERMINAL &&
            handle[2].sym_type == PREC_SYM_NONTERM ){

        bool conv_l = false;
        bool conv_r = false;
        DataType res_type;

        int sem_res = sem_eval_binary_op(handle[0].data_type, handle[2].data_type, &conv_l, &conv_r, handle[1].token_type, &res_type);

        if(sem_res != 0)    return sem_res;

        
        if(conv_l){
            codegen_int2float_second();
        }
        if(conv_r){
            codegen_int2float_top();
        }
        codegen_binary_op(handle[1].token_type, res_type);

        result_nonterm.data_type = res_type;

    }
    else{
        return 2;
    }

    
    s->top = (size_t)mark_idx;
    prec_stack_push(s, result_nonterm);
    return 0;
    
}