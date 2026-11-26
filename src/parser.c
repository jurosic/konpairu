#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "error_codes.h"
#include "parser.h"
#include "token_buffer.h"
#include "semantics.h"
#include "psa.h"
#include "codegen.h"



#define MAX_PREC_STACKS       64    //maximum amount of PrecStacks active at once
#define MAX_PARAM_COUNT       64    //maximum of alloved parameters per function 
#define TOKEN_BUFFER_INIT_CAP 8     //capacity of TokenBuffer at start


//      helper struct      //
//helper struct for function parameters
typedef struct{
    char name[LEXERLIMIT_ID_LENGTH + 1];
    DataType type;
}FuncParam;


//      parser struct       //
typedef struct{
    Lexer *lexer;
    TokenBuffer *buffer;
    bool was_mainfunc;
    DataType curr_func_ret_type;
    PrecStack *curr_stacks[MAX_PREC_STACKS];
    unsigned curr_stack_count;
}Parser;


//      forward declarations        //
static void parse_error(Parser *p, int exit_code, const char *msg);
static Token* curr_token(Parser *p);
static Token* peek_token(Parser *p);
static void advance(Parser *p);
static bool match(Parser *p, TokenType type);
static void expect(Parser *p, TokenType type);

DataType parse_expression(Parser *p);
DataType parse_func_call(Parser *p);
void parse_return(Parser *p);
void parse_assign(Parser *p);

static void parse_first_import(Parser *p);
static void parse_block(Parser *p, unsigned header_indent);
static void parse_var(Parser *p);
static void parse_func(Parser *p);
static void parse_if(Parser *p, unsigned header_indent);
static void parse_while(Parser *p, unsigned header_indent);
static void parse_statement(Parser *p, unsigned curr_indent);



//      helper funcs     //


char* mystrdup(const char* s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if (p != NULL) {
        memcpy(p, s, len);
    }
    return p;
}


//exits compiler immediately with error msg & exit_code
static void parse_error(Parser *p, int exit_code, const char *msg){
    unsigned line = 0;

    if(p){
        if(p->buffer){
            token_buffer_free(p->buffer);
        }
        if(p->lexer){
            line = p->lexer->line;
            lexer_free(p->lexer);
        }
        while(p->curr_stack_count > 0){
            p->curr_stack_count--;
            prec_stack_free(p->curr_stacks[p->curr_stack_count]);
            
        }
    }

    fprintf(stderr, "Mistake at line %d: %s\n", line, msg);
    sem_dispose();
    codegen_free();
    exit(exit_code);
}

//Returns Token* to curent token
//calls parse_error if TOKEN_INVALID
static Token* curr_token(Parser *p){
    if(p->buffer->size == 0){
        load_first_token(p->lexer, p->buffer);
    }
    if(!p->buffer->arr){
        parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
    }

    Token *t = &p->buffer->arr[0];
    if(t->type == TOKEN_INVALID){
        parse_error(p, LEXER_ERR_CODE, p->lexer->err_msg ? p->lexer->err_msg: "Lexer error");
    }
    return t;

}

//look at next token(0 curr, 1 lookahead)
static Token* peek_token(Parser *p){
    //if last token is EOF return EOF
    if(p->buffer->arr[p->buffer->size - 1].type == TOKEN_EOF){
        return &p->buffer->arr[p->buffer->size - 1];
    }
    load_next_token(p->lexer, p->buffer);
        return &p->buffer->arr[p->buffer->size - 1];
}

//advances to next token, frees all other ones
static void advance(Parser *p){
    if(p->buffer->size > 1){
        token_buffer_set_as_first(p->buffer);
    }
    else{
        load_first_token(p->lexer, p->buffer);
    }
    if(p->buffer->arr[0].type == TOKEN_INVALID){
        parse_error(p, LEXER_ERR_CODE, p->lexer->err_msg ? p->lexer->err_msg: "Lexer error");
    }
}

//returns TRUE and advances if token matches type, FALSE otherwise
static bool match(Parser *p, TokenType type){
    if(curr_token(p)->type == type){
        advance(p);
        return true;
    }
    return false;
}

//exits with error if token type doesnt match
static void expect(Parser *p, TokenType type){
    if(!match(p, type)){
        parse_error(p, SYNTAX_ERR_CODE, "Syntax error: unexpected token");
    }
}



//      parse funcs     //


DataType parse_expression(Parser *p){
    PrecStack *stack = prec_stack_init();
    p->curr_stacks[p->curr_stack_count++] = stack;

    if(!stack){
        parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
    }

    bool expr_end = false;

    while(!expr_end){
        Token *curr = curr_token(p);

        if(curr->type == TOKEN_ID && peek_token(p)->type == TOKEN_PAREN_L){
            PrecTableIndex top_term_idx = prec_stack_top_terminal_index(stack);
            if(prec_table[top_term_idx][PT_OPERAND] != '<'){
                parse_error(p, SYNTAX_ERR_CODE, "Unexpected function call in expression");
            }

            DataType ret_type = parse_func_call(p);
            PrecStackItem item = {
                .sym_type = PREC_SYM_NONTERM,
                .token_type = TOKEN_ID,
                .data_type = ret_type,
                .name = NULL
            };

            if(!prec_stack_push(stack, item)){
                parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
            }
            continue;
        }

        PrecTableIndex top_term_idx = prec_stack_top_terminal_index(stack);
        PrecTableIndex curr_term_idx = token_to_pt_index(curr->type);

        char rel = prec_table[top_term_idx][curr_term_idx];

        switch (rel){
            case '<':{
                if(!prec_stack_insert_shift(stack)){
                    parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed in precedence stack");
                }
                PrecStackItem item = {
                    .sym_type = PREC_SYM_TERMINAL,
                    .token_type = curr->type,
                    .data_type = TYPE_UNIT,
                    .name = (curr->type == TOKEN_ID) ? mystrdup(curr->value.name) : NULL,
                    .value = curr->value
                };
                if(curr->type == TOKEN_LIT_STR){
                    item.value.str_val = mystrdup(curr->value.str_val);
                }
                if(prec_stack_push(stack, item) == false){
                    parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
                }
                advance(p);
                break;
            }

            case '=':{
                PrecStackItem item = {  
                    .sym_type = PREC_SYM_TERMINAL,
                    .token_type = curr->type,
                    .data_type = TYPE_UNIT,
                    .name = (curr->type == TOKEN_ID) ? curr->value.name : NULL,
                    .value = curr->value
                };
                if(curr->type == TOKEN_LIT_STR){
                    item.value.str_val = mystrdup(curr->value.str_val);
                }
                if( (curr->type == TOKEN_ID && !item.name) || 
                    (curr->type == TOKEN_LIT_STR && !item.value.str_val) ||
                    !prec_stack_push(stack, item)){
                        free(item.name);
                        if(curr->type == TOKEN_LIT_STR){
                            free(item.value.str_val);
                        }
                        parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
                    }
                advance(p);
                break;
            }

            case '>':{
                int err = reduce_rule(stack);
                if(err != 0){
                    parse_error(p, err, "Syntax or semantic error in expression");
                }
                break;
            }

            case '$':{
                if(stack->top == 2 && stack->items[1].sym_type == PREC_SYM_NONTERM){
                    expr_end = true;
                }
                else{
                    int err = reduce_rule(stack);
                    if(err != 0){
                        parse_error(p, err, "Syntax or semantic error at end of expression");
                    }
                }
                break;
            }

            default:
                parse_error(p, SYNTAX_ERR_CODE, "Syntax error in expression");
                break;
            
        }

    }

    DataType final_type = stack->items[1].data_type;

    p->curr_stack_count--;
    prec_stack_free(stack);
    return final_type;
}


DataType parse_func_call(Parser *p){
    if(curr_token(p)->type != TOKEN_ID){
        parse_error(p, SYNTAX_ERR_CODE, "Expected function identifier");
    }

    char func_name[LEXERLIMIT_ID_LENGTH + 1] = {0};
    if(curr_token(p)->value.name){
        strcpy(func_name, curr_token(p)->value.name);
    }
    advance(p);

    if(!match(p, TOKEN_PAREN_L)){
        parse_error(p, SYNTAX_ERR_CODE, "Expected '(' after function identifier");
    }

    DataType arg_types[MAX_PARAM_COUNT];
    int arg_count = 0;

    if(curr_token(p)->type != TOKEN_PAREN_R){
        while(true){
            if(arg_count >= MAX_PARAM_COUNT){
                parse_error(p, INTERNAL_ERR_CODE, "Exceeded maximum amount of function arguments");
            }

            DataType arg_type = parse_expression(p);
            arg_types[arg_count++] = arg_type;

            if(match(p, TOKEN_COMMA)){
                if(curr_token(p)->type == TOKEN_PAREN_R)    break;
                continue;
            }
            else{
                break;
            }
        }
    }

    if(!match(p, TOKEN_PAREN_R)){
        parse_error(p, SYNTAX_ERR_CODE, "Expected ')' after function arguments");
    }

    DataType return_type;
    int sem_res = sem_check_function_call(func_name, arg_count, arg_types, &return_type);
     if(sem_res != 0){
        parse_error(p, sem_res, "SYN or SEM error");
    }

    //codegen built in finctions
    if(strcmp(func_name, "print") == 0)     codegen_builtin_print(arg_types[0]);
    else if(strcmp(func_name, "readInt") == 0)     codegen_builtin_read_int();
    else if(strcmp(func_name, "readDouble") == 0)     codegen_builtin_read_double();
    else if(strcmp(func_name, "readLine") == 0)     codegen_builtin_read_line();

    else if(strcmp(func_name, "toStr") == 0)     codegen_builtin_to_str(arg_types[0]);
    else if(strcmp(func_name, "toInt") == 0)     codegen_builtin_to_int();

    else if(strcmp(func_name, "strLength") == 0)     codegen_builtin_str_length();

    else if(strcmp(func_name, "chr") == 0)     codegen_builtin_chr();
    else if(strcmp(func_name, "ord") == 0)     codegen_builtin_ord();

    else if(strcmp(func_name, "strcmp") == 0)     codegen_builtin_strcmp();
    else if(strcmp(func_name, "subStr") == 0)     codegen_builtin_substr();
    else    codegen_call_func(func_name);

    return return_type;
}


void parse_return(Parser *p){
    expect(p, TOKEN_RETURN);

    DataType expr_type = TYPE_UNIT;
    TokenType t = curr_token(p)->type;

    if(t != TOKEN_NEWLINE && t != TOKEN_EOF){
        expr_type = parse_expression(p);
    }
    else{
        codegen_push_unit();
    }

    int sem_res = sem_check_return(p->curr_func_ret_type, expr_type);
    if(sem_res != 0){
        parse_error(p, sem_res, "Semantic error in return statement type matching");
    }

    codegen_func_return();
}


void parse_assign(Parser *p){
    if(curr_token(p)->type != TOKEN_ID){
        parse_error(p, SYNTAX_ERR_CODE, "Expected variable identifier in assigment");
    }

    char var_name[LEXERLIMIT_ID_LENGTH + 1] = {0};
    if(curr_token(p)->value.name){
        strcpy(var_name, curr_token(p)->value.name);
    }

    advance(p);

    expect(p, TOKEN_ASSIGN);

    DataType expr_type = parse_expression(p);

    DataType var_type;
    sem_get_variable_type(var_name, &var_type);
    if(var_type == TYPE_DOUBLE && expr_type == TYPE_INT){
        codegen_int2float_top();
    }

    int sem_res = sem_check_assignment(var_name, expr_type);
    if(sem_res != 0){
        parse_error(p, sem_res, "Semantic error in assigment");
    }

    int scope_id = sem_get_scope_id();
    codegen_pop_to_var(var_name, scope_id);

}


//parses the mandatory start of file import
static void parse_first_import(Parser *p){

    expect(p, TOKEN_IMPORT);
    Token *id = curr_token(p);

    if(id->type != TOKEN_ID || strcmp(id->value.name, "ifj26") != 0){
        parse_error(p, SYNTAX_ERR_CODE, "Expected import \"ifj26\"");
    }
    advance(p);
    expect(p, TOKEN_DOT);
    expect(p, TOKEN_MUL);
}

//parses statements inside func/if/while
//expect header_indent = indent of caller of this function
static void parse_block(Parser *p, unsigned header_indent){

    //NEWLINE check
    if(curr_token(p)->type != TOKEN_NEWLINE){
        parse_error(p, SYNTAX_ERR_CODE, "expected NEWLINE befory block body");
    }

    //expected indent of block
    unsigned block_indent = curr_token(p)->value.indent_weight;
    if(block_indent <= header_indent){
        parse_error(p, SYNTAX_ERR_CODE, "Block indent must be grater than header indent");
    }

    //expects atleast one stement in block
    bool parsed_one = false;

    //loop for parse_statement
    while(curr_token(p)->type != TOKEN_EOF){
        //advance from NEWLINE to statement
        advance(p);
        parse_statement(p, block_indent);
        parsed_one = true;

        //indent check
        if(curr_token(p)->type == TOKEN_NEWLINE){
            if(curr_token(p)->value.indent_weight < block_indent){
                break;
            }
        }
        //missing NEWLINE between statements
        else if(curr_token(p)->type != TOKEN_EOF){
            parse_error(p, SYNTAX_ERR_CODE, "Expected NEWLINE between statements");
        }
    }

    if(!parsed_one){
        parse_error(p, SYNTAX_ERR_CODE, "Block has to have at least one statement");
    }

}



//parses val or var
static void parse_var(Parser *p){
    //check if mutable (val or var)
    bool is_mutable = (curr_token(p)->type == TOKEN_VAR);
    advance(p);

    //var name
    if(curr_token(p)->type != TOKEN_ID){
        parse_error(p, SYNTAX_ERR_CODE, "Expected variable identifier in declaration");
    }

    char var_name[LEXERLIMIT_ID_LENGTH + 1] = {0};
    if(curr_token(p)->value.name){
        strcpy(var_name, curr_token(p)->value.name);
    }

    advance(p);

    //optional var type
    DataType expl_type = TYPE_UNIT;
    if(match(p, TOKEN_COLON)){
        if(     match(p, TOKEN_INT))    expl_type = TYPE_INT;
        else if(match(p, TOKEN_DOUBLE)) expl_type = TYPE_DOUBLE;
        else if(match(p, TOKEN_STRING)) expl_type = TYPE_STRING;
        else    parse_error(p, SYNTAX_ERR_CODE, "Unknown data type");
    }

    expect(p, TOKEN_ASSIGN);


    //var actual type
    DataType expr_type = parse_expression(p);
    DataType final_type;

    //final_type logic
    if(expl_type != TYPE_UNIT){
        if(expl_type == expr_type){
            final_type = expl_type;
        }
        else if(expl_type == TYPE_DOUBLE && expr_type == TYPE_INT){
            final_type = TYPE_DOUBLE;
            codegen_int2float_top();
        }
        else{
            parse_error(p,SEM_ERR_CODE_DEF_TYPE, "Variable declared type differs from expression type");
        }
    }
    //TO DO: optimize logic
    else{
        final_type = expr_type;
    }

    if(final_type == TYPE_UNIT){
        parse_error(p, SEM_ERR_CODE_DEF_TYPE, "Variable cannot be of type Unit");
    }


    //semantics control
    SymbolData sym;
    sym.symbol_type = SYMBOL_VAR;
    sym.type = final_type;
    sym.data.var_data.is_mutable = is_mutable;

    int sem_res = sem_define_variable(var_name, sym, is_mutable);
    if(sem_res != 0){
        parse_error(p, sem_res, "Semantics error durring declaration");
    }

    //codegen
    int scope_id = sem_get_scope_id();
    codegen_def_var(var_name, scope_id);
    codegen_pop_to_var(var_name, scope_id);
}

//parses whole function
static void parse_func(Parser *p){

    //main check
    bool is_main = false;
    if(match(p, TOKEN_MAINFUNC)){
        is_main = true;
        if(p->was_mainfunc){
            parse_error(p, SEM_ERR_CODE_REDEF, "Redefinition of main function @main");
        }
        p->was_mainfunc = true;
    }

    //func name
    expect(p, TOKEN_DEF);

    if(curr_token(p)->type != TOKEN_ID){
        parse_error(p, SYNTAX_ERR_CODE, "Expected function name");
    }

    char func_name[LEXERLIMIT_ID_LENGTH + 1] = {0};
    if(curr_token(p)->value.name){
        strcpy(func_name, curr_token(p)->value.name);
    }
    advance(p);


    //params parse
    if(!match(p, TOKEN_PAREN_L)){
        parse_error(p, SYNTAX_ERR_CODE, "Expected '(");
    }

    FuncParam param[MAX_PARAM_COUNT];
    int param_count = 0;

    if(curr_token(p)->type != TOKEN_PAREN_R){
        //asign parameters to param[param_count]
        while(true){
            //error if to many parameters
            if(param_count >= MAX_PARAM_COUNT){
                parse_error(p, INTERNAL_ERR_CODE, "Too many parameters for function");
            }

            //erorr if no name
            if(curr_token(p)->type != TOKEN_ID){
                parse_error(p, SYNTAX_ERR_CODE, "Expected parameter name");
            }

            memset(param[param_count].name, 0, LEXERLIMIT_ID_LENGTH + 1);
            if(curr_token(p)->value.name){
                strcpy(param[param_count].name, curr_token(p)->value.name);
            }
            

            advance(p);
            expect(p, TOKEN_COLON);

            if(match(p, TOKEN_INT))         param[param_count].type = TYPE_INT;
            else if(match(p, TOKEN_DOUBLE))    param[param_count].type = TYPE_DOUBLE;
            else if(match(p, TOKEN_STRING))    param[param_count].type = TYPE_STRING;
            else if(match(p, TOKEN_UNIT))    param[param_count].type = TYPE_UNIT;
            else    parse_error(p, SYNTAX_ERR_CODE, "Unknown parameter type");

            param_count++;

            //TO DO: optimize logic
            if(match(p, TOKEN_COMMA)){
                if(curr_token(p)->type == TOKEN_PAREN_R)    break;
                continue;
            }
            else    break; 

        }
    }
    
    advance(p);

    //optional return type
    DataType return_type = TYPE_UNIT;
    if(match(p, TOKEN_COLON)){
        if(     match(p, TOKEN_INT))    return_type = TYPE_INT;
        else if(match(p, TOKEN_DOUBLE)) return_type = TYPE_DOUBLE;
        else if(match(p, TOKEN_STRING)) return_type = TYPE_STRING;
        else if(match(p, TOKEN_UNIT))   return_type = TYPE_UNIT;
        else    parse_error(p, SYNTAX_ERR_CODE, "Unknown function return type");
    }

    //semantics check for main
    if(is_main){
        if(param_count > 0){
            parse_error(p, SEM_ERR_CODE_ARGS, "@main function has too many parameters");
        }
        if(return_type != TYPE_UNIT){
            parse_error(p, SEM_ERR_CODE_DEF_TYPE, "@main function must have return type Unit");
        }
    }

    //alloc param_types for sym table
    DataType *param_types = NULL;
    if(param_count > 0){
        param_types = malloc(param_count * sizeof(DataType));
        if(!param_types){
            parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
        }
        for(int i = 0; i < param_count; i++){
            param_types[i] = param[i].type;
        }
    }

    //register func in sym table
    int sem_res = sem_define_function(func_name, return_type, param_count, param_types);
    if(param_types)     free(param_types);
    if(sem_res != 0){
        parse_error(p, sem_res, "Semantics error in function definition");
    }


    expect(p, TOKEN_ASSIGN);
    if(is_main && codegen_main_start() != 0){
            parse_error(p, INTERNAL_ERR_CODE, "Codegen main start failed");
    }

    else if(codegen_func_start(func_name) != 0){
            parse_error(p, INTERNAL_ERR_CODE, "Codegen func start failed");
    }

    sem_enter_scope();

    int scope_id = sem_get_scope_id();

    //register parameters to sem
    for(int i = 0; i < param_count; i++){
        SymbolData param_sym = {
            .symbol_type = SYMBOL_VAR,
            .type = param[i].type,
            .data.var_data.is_mutable = false
        };

        int p_res = sem_define_variable(param[i].name, param_sym, false);
        if(p_res != 0){
            parse_error(p, p_res, "Semantic error with registering parameter of func");
        }
    }

    //codegen params
    for(int i = param_count - 1; i >= 0; i--){
        codegen_func_param(param[i].name, scope_id);
    }

    p->curr_func_ret_type = return_type;
    
    if(curr_token(p)->type == TOKEN_NEWLINE){
        parse_block(p, 0);
    }
    //single line function
    else{
        DataType expr_type = parse_expression(p);
        if(return_type == TYPE_DOUBLE && expr_type == TYPE_INT){
            codegen_int2float_top();
            expr_type = TYPE_DOUBLE;
        }
        int ret_sem = sem_check_return(return_type, expr_type);
        if(ret_sem != 0){
            parse_error(p, ret_sem, "Return type mismatch in singleline function");
        }
        codegen_func_return();
    }

    sem_leave_scope();

    if(is_main){
        codegen_main_end();
    }
    else{
        codegen_func_end();
    }
}

//parses if statement
static void parse_if(Parser *p, unsigned header_indent){
    expect(p, TOKEN_IF);

    int label_id = codegen_next_label_id();
    
    DataType cond_type = parse_expression(p);
    if(cond_type != TYPE_BOOLEAN){
        parse_error(p, SEM_ERR_CODE_DEF_TYPE, "If condition must be of type Boolean");
    }

    codegen_if_start(label_id);
    expect(p, TOKEN_THEN);

    sem_enter_scope();
    parse_block(p, header_indent);
    sem_leave_scope();

    codegen_if_else(label_id);

    if(curr_token(p)->type == TOKEN_NEWLINE){
        advance(p);
    }
    expect(p, TOKEN_ELSE);

    sem_enter_scope();
    parse_block(p, header_indent);
    sem_leave_scope();
    
    codegen_if_end(label_id);
}

//parses while statement
static void parse_while(Parser *p, unsigned header_indent){
    expect(p, TOKEN_WHILE);

    int label_id = codegen_next_label_id();
    codegen_while_header(label_id);

    DataType cond_type = parse_expression(p);
    if(cond_type != TYPE_BOOLEAN){
        parse_error(p, SEM_ERR_CODE_DEF_TYPE, "Condition in 'While' must be Boolean type");
    }

    codegen_while_body_start(label_id);

    expect(p, TOKEN_DO);
    sem_enter_scope();
    parse_block(p, header_indent);
    sem_leave_scope();

    codegen_while_end(label_id);
}


void parse_statement(Parser *p, unsigned curr_indent){
    TokenType t = curr_token(p)->type;

    switch (t){
        case TOKEN_VAL:
        case TOKEN_VAR:
            parse_var(p);
            break;

        case TOKEN_IF:
            parse_if(p, curr_indent);
            break;

        case TOKEN_WHILE:
            parse_while(p, curr_indent);
            break;

        case TOKEN_RETURN:
            (void)parse_return(p);
            break;

        case TOKEN_ID:{
            Token *next = peek_token(p);
            if(next->type == TOKEN_ASSIGN){
                parse_assign(p);
            }
            else if(next->type == TOKEN_PAREN_L){
                (void)parse_func_call(p);
                codegen_discard_stack_top();
            }
            else{
                parse_expression(p);
                codegen_discard_stack_top();
            }
            break;
        }

        case TOKEN_NEWLINE:
            advance(p);
            break;

        default:
            parse_expression(p);
            codegen_discard_stack_top();
            break;
    }
}


//parses the entire code
//use this in main.c
int parse(Lexer *lexer){
    if(!lexer){
        return INTERNAL_ERR_CODE;
    }

    if(codegen_init() != 0) return INTERNAL_ERR_CODE;

    Parser p;
    p.lexer = lexer;
    p.was_mainfunc = false;
    p.curr_func_ret_type = TYPE_UNIT;
    p.buffer = token_buffer_init(TOKEN_BUFFER_INIT_CAP);   //capacity of TokenBuffer at start
    p.curr_stack_count = 0;

    if(!p.buffer){
        parse_error(&p, INTERNAL_ERR_CODE, "Memory allocation failed");
    }

    sem_init();
    load_first_token(p.lexer, p.buffer);

    parse_first_import(&p);
    codegen_program_start();

    while(curr_token(&p)->type != TOKEN_EOF){
        if(curr_token(&p)->type == TOKEN_NEWLINE){
            if(curr_token(&p)->value.indent_weight != 0){
                parse_error(&p, SYNTAX_ERR_CODE, "Expected indent of 0");
            }
            advance(&p);
            continue;
        }
        parse_func(&p);
    }

    if(!p.was_mainfunc){
        parse_error(&p, SEM_ERR_CODE_NO_DEF, "Semantics error: missing function @main");
    }

    codegen_flush();
    codegen_free();
    token_buffer_free(p.buffer);
    sem_dispose();

    return 0;

}