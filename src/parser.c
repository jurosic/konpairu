#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "parser.h"
#include "token_buffer.h"
#include "semantics.h"



#define MAX_PARAM_COUNT       64    //maximum of alloved parameters per function 
#define TOKEN_BUFFER_INIT_CAP 8     //capacity of TokenBuffer at start


//      error codes     //
#define LEXER_ERR_CODE          1           //lexical
#define SYNTAX_ERR_CODE         2           //in syntax
#define SEM_ERR_CODE_NO_DEF     3           //missing var/val/func def or no @main
#define SEM_ERR_CODE_REDEF      4           //val/var/func is redifined
#define SEM_ERR_CODE_ARGS       5           //arguments count/type 
#define SEM_ERR_CODE_DEF_TYPE   6           //val/var/func/ret type 
#define SEM_ERR_CODE_ASSIGN     7           //assign value to const
#define SEM_ERR_CODE_MISC       10          //miscellaneous
#define INTERNAL_ERR_CODE       99          //compiler error (mastly mem alloc) 



//      helper structs      //
//helper struct for function parameters
typedef struct{
    char *name;
    DataType type;
}FuncParam;


//      parser struct       //
typedef struct{
    Lexer *lexer;
    TokenBuffer *buffer;
    bool was_mainfunc;
    unsigned indent_lvl;
}Parser;


//      helper funcs     //
//exits compiler with error msg & exit_code
static void parse_error(Parser *p, int exit_code, const char *msg){
    fprintf(stderr, "Mistake at line %d: %s\n", p->lexer ? p->lexer->line : 0, msg);

    if(p->buffer){
        token_buffer_free(p->buffer);
    }
    sem_dispose();
    exit(exit_code);
}

//loads token to the start of buffer, exits if TOKEN_INVALID
static Token* curr_token(Parser *p){
    if(p->buffer->size == 0){
        load_first_token(p->lexer, p->buffer);
    }

    Token *t = &p->buffer->arr;
    if(t->type == TOKEN_INVALID){
        parse_error(p, LEXER_ERR_CODE, p->lexer->err_msg ? p->lexer->err_msg: "Lexer mistake");
    }
    return t;

}

//look at token at idx (0 curr, 1 lookahead,...)
static Token* peek_token(Parser *p, size_t idx){
    while (p->buffer->size <= idx){
        load_next_token(p->lexer, p->buffer);

        if(!p->buffer){
            parse_error(p, INTERNAL_ERR_CODE, "Internal error: memory allocation failed");
        }
    }

    Token *t = &p->buffer->arr[idx];
    if(t->type == TOKEN_INVALID){
        parse_error(p, LEXER_ERR_CODE, p->lexer->err_msg ? p->lexer->err_msg: "Lexer mistake");
    }
    return t;
}

static void advance(Parser *p){
    if(p->buffer->size > 1){
        token_buffer_set_as_first(p->buffer);
    }
    else{
        load_first_token(p->lexer, p->buffer);
    }
    if(p->buffer->arr[0].type == TOKEN_INVALID){
        parse_error(p, LEXER_ERR_CODE, p->lexer->err_msg ? p->lexer->err_msg: "Lexer mistake");
    }
}

//returns TRUE if token matches type, FALSE otherwise
static bool match(Parser *p, TokenType type){
    if(curr_token(p)->type == type){
        advance(p);
        return true;
    }
    return false;
}

static void expect(Parser *p, TokenType type){
    if(!match(p, type)){
        parse_error(p, SYNTAX_ERR_CODE, "syntax mistake: unecpected token");
    }
}

//not sure if needed
//skips useless newlines
static void skip_newlines(Parser *p){
    while(curr_token(p)->type == TOKEN_NEWLINE){
        advance(p);
    }
}

//      parse funcs     //

static DataType parse_expression(Parser *p);


static void parse_block(Parser *p, unsigned header_indent){
    if(curr_token(p)->type == TOKEN_NEWLINE){
        advance(p);
    }

    skip_newlines(p);

    if(curr_token(p)->type == TOKEN_EOF){
        parse_error(p, SYNTAX_ERR_CODE, "Block cant be empty");
    }

    unsigned block_indent = curr_token(p)->value.indent_weight;
    if(block_indent <= header_indent){
        parse_error(p, SYNTAX_ERR_CODE, "Indent of block should be bigger then header indent");
    }

    bool parsed_one = false;

    while(curr_token(p)->type != TOKEN_EOF){
        if(curr_token(p)->type == TOKEN_NEWLINE){
            unsigned curr_indent = curr_token(p)->value.indent_weight;

            if(curr_indent < block_indent)  break;

            advance(p);
            continue;
        }

        parse_statement(p);
        parsed_one = true;

        if(curr_token(p)->type == TOKEN_NEWLINE){
            if(curr_token(p)->value.indent_weight < block_indent)   break;
        }
    }

    if(!parsed_one){
        parse_error(p, SYNTAX_ERR_CODE, "Block has to have at least one statement");
    }

}



static void parse_first_import(Parser *p){
    expect(p, TOKEN_IMPORT);
    Token *id = curr_token(p);
    if(id->type != TOKEN_ID || strcmp(id->value.name, "ifj26.*") != 0){
        parse_error(p, SYNTAX_ERR_CODE, "Expected import \"ifj26.*\"");
    }
    advance(p);
}



//TO DO: rename and use var insted of val/var
static void parse_val_var(Parser *p){
    //check if mutable
    bool is_mutable = (curr_token(p)->type == TOKEN_VAR);
    advance(p);

    //val/var name
    if(curr_token(p)->type != TOKEN_ID){
        parse_error(p, SYNTAX_ERR_CODE, "expected identifier of variable");
    }

    char *var_name = strdup(curr_token(p)->value.name);
    advance(p);

    //optional val/var type
    DataType expl_type = TYPE_UNIT;
    if(match(p, TOKEN_COLON)){
        if(     match(p, TOKEN_INT))    expl_type = TYPE_INT;
        else if(match(p, TOKEN_DOUBLE)) expl_type = TYPE_DOUBLE;
        else if(match(p, TOKEN_STRING)) expl_type = TYPE_STRING;
        else parse_error(p, SYNTAX_ERR_CODE, "Unknown data type");
    }

    expect(p, TOKEN_ASSIGN);


    //val/var actual type
    DataType expr_type = parse_expression;
    DataType final_type;

    if(expl_type != TYPE_UNIT){
        if(expl_type == expr_type){
            final_type = expl_type;
        }
        else if(expl_type == TYPE_DOUBLE && expr_type == TYPE_INT){
            final_type == TYPE_DOUBLE;
        }
        else{
            free(var_name);
            parse_error(p,SEM_ERR_CODE_DEF_TYPE, "val/var type differs from expected");
        }
    }
    //TO DO: optimize logic
    else{
        final_type == expr_type;
    }

    if(final_type == TYPE_UNIT){
        free(var_name);
        parse_error(p, SEM_ERR_CODE_DEF_TYPE, "var cannot be type Unit");
    }


    //semantics control
    SymbolData sym;
    sym.symbol_type = SYMBOL_VAR;
    sym.type = final_type;
    sym.data.var_data.is_mutable = is_mutable;

    int sem_res = sem_define_variable(var_name, sym, is_mutable);
    free(var_name);
    if(sem_res != 0){
        parse_error(p, sem_res, "Semantics error durring declaration");
    }
}



static void parse_func(Parser *p){
    unsigned header_indent = curr_token(p)->value.indent_weight;

    //main check
    bool is_main = false;
    if(match(p, TOKEN_MAINFUNC)){
        is_main = true;
        if(p->was_mainfunc){
            parse_error(p, SEM_ERR_CODE_REDEF, "Redefining of function @main");
        }
        p->was_mainfunc = true;
    }

    //func name
    expect(p, TOKEN_DEF);
    if(curr_token(p)->type != TOKEN_ID){
        parse_error(p, SYNTAX_ERR_CODE, "expected function name");
    }

    char *func_name = curr_token(p)->value.name;
    advance(p);


    //params parse
    expect(p, TOKEN_PAREN_L);

    FuncParam param[MAX_PARAM_COUNT];
    unsigned param_count = 0;

    if(curr_token(p)->type != TOKEN_PAREN_R){
        //TO DO: funguje, ale while(true) je evil
        while(true){
            if(param_count >= 64){
                parse_error(p, INTERNAL_ERR_CODE, "Too many parameters for a function");
            }

            if(curr_token(p)->type != TOKEN_ID){
                parse_error(p, SYNTAX_ERR_CODE, "Expected parameter name");
            }

            param[param_count].name = curr_token(p)->value.name;
            advance(p);
            expect(p, TOKEN_COLON);

            if(match(p, TOKEN_INT))         param[param_count].type = TYPE_INT;
            else if(match(p, TOKEN_DOUBLE))    param[param_count].type = TYPE_DOUBLE;
            else if(match(p, TOKEN_STRING))    param[param_count].type = TYPE_STRING;
            else if(match(p, TOKEN_UNIT))    param[param_count].type = TYPE_UNIT;
            else{
                parse_error(p, SYNTAX_ERR_CODE, "Unknown type of parameter");
            }

            param_count++;

            //TO DO: optimize logic
            if(match(p, TOKEN_COMMA)){
                if(curr_token(p)->type == TOKEN_PAREN_R)    break;
                continue;
            }
            else    break;

        }
    }
    expect(p, TOKEN_PAREN_R);


    //optional return type
    DataType return_type = TYPE_UNIT;
    if(match(p, TOKEN_COLON)){
        if(     match(p, TOKEN_INT))    return_type = TYPE_INT;
        else if(match(p, TOKEN_DOUBLE)) return_type = TOKEN_DOUBLE;
        else if(match(p, TOKEN_STRING)) return_type = TOKEN_STRING;
        else if(match(p, TOKEN_UNIT))   return_type = TOKEN_UNIT;
        else {
            parse_error(p, SYNTAX_ERR_CODE, "Unknown function return type");
        }
    }

    //semantics check for main
    if(is_main){
        if(param_count > 0){
            parse_error(p, SEM_ERR_CODE_ARGS, "@main has too many arguments");
        }
        if(return_type != TYPE_UNIT){
            parse_error(p, SEM_ERR_CODE_DEF_TYPE, "function @main has to have return type Unit");
        }
    }

    //TO DO: works, but make it without malloc if possible
    //alloc param_types for sym table
    DataType *param_types = NULL;
    if(param_count > 0){
        param_types = malloc(param_count * sizeof(DataType));
        if(!param_types){
            parse_error(p, INTERNAL_ERR_CODE, "Memory allocation for func parameters failed");
        }
        for(unsigned i = 0; i < param_count; i++){
            param_types[i] = param[i].type;
        }
    }

    //register func in sym table
    int sem_res = sem_define_function(func_name, return_type, param_count, param_types);
    if(param_types)     free(param_types);
    if(sem_res != 0){
        parse_error(p, sem_res, "Semantics error: func definition");
    }


    expect(p, TOKEN_ASSIGN);
    sem_enter_scope();

    //add parameters as val
    for(unsigned i = 0; i < param_count; i++){
        SymbolData param_sym;
        param_sym.symbol_type = SYMBOL_VAR;
        param_sym.type = param[i].type;
        param_sym.data.var_data.is_mutable = false;

        int p_res = sem_define_variable(param[i].name, param_sym, false);
        if(p_res != 0){
            parse_error(p, p_res, "Semantic error with registering parameter of func");
        }
    }

    parse_block(p, header_indent);
    sem_leave_scope();
}

static void parse_if(Parser *p){
    unsigned header_indent = curr_token(p)->value.indent_weight;
    expect(p, TOKEN_IF);

    DataType cond_type = parse_expression(p);
    if(cond_type != TYPE_BOOLEAN){
        parse_error(p, SEM_ERR_CODE_DEF_TYPE, "Condition if 'if' must be Boolean type");
    }

    expect(p, TOKEN_THEN);
    sem_enter_scope();
    parse_block(p, header_indent);
    sem_leave_scope();

    expect(p, TOKEN_ELSE);
    sem_enter_scope();
    parse_block(p, header_indent);
    sem_leave_scope();
    
}

static void parse_while(Parser *p){
    unsigned header_indent = curr_token(p)->value.indent_weight;
    expect(p, TOKEN_WHILE);

    DataType cond_type = parse_expression(p);
    if(cond_type != TYPE_BOOLEAN){
        parse_error(p, SEM_ERR_CODE_DEF_TYPE, "Condition in 'While' must be Boolean type");
    }

    expect(p, TOKEN_DO);
    sem_enter_scope();
    parse_block(p, header_indent);
    sem_leave_scope();
}




static void parse_statement(Parser *p){
    TokenType t = curr_token(p)->type;

    switch (t){
        case TOKEN_VAL:
        case TOKEN_VAR:
            parse_val_var(p);
            break;

        /*//shouldnt be here, should be at top level
        case TOKEN_DEF:
        case TOKEN_MAINFUNC:
            parse_func(p);
            break;*/
        case TOKEN_IF:
            parse_if(p);
            break;
        case TOKEN_WHILE:
            parse_while(p);
            break;
        case TOKEN_RETURN:
            parse_return(p);
            break;
        case TOKEN_ID:
            Token *next = peek_token(p, 1);
            if(next->type == TOKEN_ASSIGN){
                parse_assign(p);
            }
            else if(next->type == TOKEN_PAREN_L){
                parse_func_call(p);
            }
            else{
                parse_error(p, SYNTAX_ERR_CODE, "Unexpected expresion after identifier")''
            }
            break;
        
        case TOKEN_NEWLINE:
            advance(p);
            break;
        default:
            parse_error(p, SYNTAX_ERR_CODE, "Unknown statement");
            break;
    }
}


//parses the entire code
//use this in main.c
int parse(Lexer *lexer){
    if(!lexer){
        return INTERNAL_ERR_CODE;
    }

    Parser p;
    p.lexer = lexer;
    p.was_mainfunc = false;
    p.indent_lvl = 0;

    p.buffer = token_buffer_init(TOKEN_BUFFER_INIT_CAP);   //capacity of TokenBuffer at start
    if(!p.buffer){
        fprintf(stderr, "Internal error: memory allocation failed in TokenBuffer\n");
        return INTERNAL_ERR_CODE;
    }

    sem_init();
    load_first_token(p.lexer, p.buffer);

    parse_first_import(&p);



    while(curr_token(&p)->type != TOKEN_EOF){
        parse_statement(&p);
    }



    if(!p.was_mainfunc){
        parse_error(&p, SEM_ERR_CODE_NO_DEF, "Semantics error: missing function @main");
    }
    token_buffer_free(p.buffer);
    sem_dispose();

    return 0;

}