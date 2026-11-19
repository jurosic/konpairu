#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "parser.h"
#include "token_buffer.h"
#include "semantics.h"
#include "psa.h"

#define MAX_PARAM_COUNT       64    // maximum allowed parameters per function
#define TOKEN_BUFFER_INIT_CAP 8     // initial capacity of TokenBuffer

//      error codes     //
#define LEXER_ERR_CODE          1   // lexical
#define SYNTAX_ERR_CODE         2   // syntax
#define SEM_ERR_CODE_NO_DEF     3   // missing var/val/func def or no @main
#define SEM_ERR_CODE_REDEF      4   // val/var/func redefined
#define SEM_ERR_CODE_ARGS       5   // arguments count/type
#define SEM_ERR_CODE_DEF_TYPE   6   // val/var/func/ret type
#define SEM_ERR_CODE_ASSIGN     7   // assign value to const
#define SEM_ERR_CODE_MISC       10  // miscellaneous
#define INTERNAL_ERR_CODE       99  // compiler error (mostly mem alloc)

//      helper structs      //
typedef struct {
    char *name;
    DataType type;
} FuncParam;

//      parser struct       //
typedef struct {
    Lexer *lexer;
    TokenBuffer *buffer;
    bool was_mainfunc;
    DataType curr_func_ret_type;
} Parser;

//      helper funcs     //

char *mystrdup(const char *s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if (p != NULL) {
        memcpy(p, s, len);
    }
    return p;
}

static void free_params(FuncParam *params, unsigned count) {
    for (unsigned i = 0; i < count; i++) {
        free(params[i].name);
        params[i].name = NULL;
    }
}

// exits compiler with error msg, current token type & exit_code
static void parse_error(Parser *p, int exit_code, const char *msg) {
    int line = (p && p->lexer) ? p->lexer->line : 0;
    if (p && p->buffer && p->buffer->arr && p->buffer->size > 0) {
        fprintf(stderr, "Mistake at line %d (token %d): %s\n",
                line, (int)p->buffer->arr[0].type, msg);
    } else {
        fprintf(stderr, "Mistake at line %d: %s\n", line, msg);
    }

    if (p && p->buffer) {
        token_buffer_free(p->buffer);
        p->buffer = NULL;
    }
    codegen_free();
    sem_dispose();
    // TODO: call lexer_free(p->lexer) here if your lexer module provides one
    exit(exit_code);
}

// Returns Token* to current token; calls parse_error if TOKEN_INVALID
static Token *curr_token(Parser *p) {
    if (p->buffer->size == 0) {
        load_first_token(p->lexer, p->buffer);
    }
    if (!p->buffer->arr) {
        parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
    }

    Token *t = p->buffer->arr;
    if (t->type == TOKEN_INVALID) {
        parse_error(p, LEXER_ERR_CODE,
                    p->lexer->err_msg ? p->lexer->err_msg : "Lexer mistake");
    }
    return t;
}

// Look at next token (index 0 = curr, index 1 = lookahead)
static Token *peek_token(Parser *p) {
    curr_token(p); // ensure index 0 is loaded
    if (p->buffer->size < 2 && p->buffer->arr[0].type != TOKEN_EOF) {
        load_next_token(p->lexer, p->buffer);
    }
    if (p->buffer->size >= 2) {
        return &p->buffer->arr[1];
    }
    return &p->buffer->arr[0];
}

// Advances to next token
static void advance(Parser *p) {
    if (p->buffer->size > 1) {
        token_buffer_set_as_first(p->buffer);
    } else {
        load_first_token(p->lexer, p->buffer);
    }
    if (!p->buffer->arr) {
        parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
    }
    if (p->buffer->arr[0].type == TOKEN_INVALID) {
        parse_error(p, LEXER_ERR_CODE,
                    p->lexer->err_msg ? p->lexer->err_msg : "Lexer mistake");
    }
}

// Returns true and advances if token matches type, false otherwise
static bool match(Parser *p, TokenType type) {
    if (curr_token(p)->type == type) {
        advance(p);
        return true;
    }
    return false;
}

// Exits with syntax error if token type doesn't match
static void expect(Parser *p, TokenType type) {
    if (!match(p, type)) {
        parse_error(p, SYNTAX_ERR_CODE, "syntax mistake: unexpected token");
    }
}

//      forward declarations     //

static void parse_statement(Parser *p, unsigned current_indent);
DataType parse_expression(Parser *p);
DataType parse_func_call(Parser *p);

// Parses either a function call or a PSA expression
static DataType parse_expr_or_call(Parser *p) {
    if (curr_token(p)->type == TOKEN_ID && peek_token(p)->type == TOKEN_PAREN_L) {
        return parse_func_call(p);
    }
    if (curr_token(p)->type == TOKEN_PAREN_L && peek_token(p)->type == TOKEN_PAREN_R) {
        advance(p); // consume '('
        advance(p); // consume ')'
        codegen_push_unit();
        return TYPE_UNIT;
    }
    return parse_expression(p);
}

//      parse funcs     //

DataType parse_expression(Parser *p) {
    PrecStack *stack = prec_stack_init();
    if (!stack) {
        parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
    }

    bool expr_end = false;

    while (!expr_end) {
        Token *curr = curr_token(p);

        PrecTableIndex top_term_idx = prec_stack_top_terminal_index(stack);
        PrecTableIndex curr_term_idx = token_to_pt_index(curr->type);

        char rel = prec_table[top_term_idx][curr_term_idx];

        switch (rel) {
            case '<':
                if (!prec_stack_insert_shift(stack)) {
                    prec_stack_free(stack);
                    parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
                }
                // fallthrough
            case '=': {
                PrecStackItem item = {
                    .sym_type = PREC_SYM_TERMINAL,
                    .token_type = curr->type,
                    .data_type = TYPE_UNIT,
                    .name = (curr->type == TOKEN_ID) ? curr->value.name : NULL,
                    .value = curr->value
                };
                prec_stack_push(stack, item);
                advance(p);
                break;
            }

            case '>': {
                int err = reduce_rule(stack);
                if (err != 0) {
                    prec_stack_free(stack);
                    parse_error(p, err, "SYN or SEM error in expression");
                }
                break;
            }

            case '$':
                if (stack->top == 2 && stack->items[1].sym_type == PREC_SYM_NONTERM) {
                    expr_end = true;
                } else {
                    int err = reduce_rule(stack);
                    if (err != 0) {
                        prec_stack_free(stack);
                        parse_error(p, err, "SYN or SEM error in expression");
                    }
                }
                break;

            default:
                prec_stack_free(stack);
                parse_error(p, SYNTAX_ERR_CODE, "SYN error in expression");
                break;
        }
    }

    DataType final_type = stack->items[1].data_type;
    prec_stack_free(stack);
    return final_type;
}

DataType parse_func_call(Parser *p) {
    if (curr_token(p)->type != TOKEN_ID) {
        parse_error(p, SYNTAX_ERR_CODE, "Expected function identifier");
    }

    char *func_name = mystrdup(curr_token(p)->value.name);
    if (!func_name) {
        parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
    }
    advance(p);

    if (!match(p, TOKEN_PAREN_L)) {
        free(func_name);
        parse_error(p, SYNTAX_ERR_CODE, "Expected '('");
    }

    DataType arg_types[MAX_PARAM_COUNT];
    int arg_count = 0;

    if (curr_token(p)->type != TOKEN_PAREN_R) {
        while (true) {
            if (arg_count >= MAX_PARAM_COUNT) {
                free(func_name);
                parse_error(p, INTERNAL_ERR_CODE, "Exceeded maximum amount of parameters");
            }

            DataType arg_type = parse_expression(p);
            arg_types[arg_count] = arg_type;
            arg_count++;

            if (match(p, TOKEN_COMMA)) {
                if (curr_token(p)->type == TOKEN_PAREN_R) {
                    break;
                }
                continue;
            } else {
                break;
            }
        }
    }

    if (!match(p, TOKEN_PAREN_R)) {
        free(func_name);
        parse_error(p, SYNTAX_ERR_CODE, "Expected ')'");
    }

    DataType return_type;
    int sem_res = sem_check_function_call(func_name, arg_count, arg_types, &return_type);
    if (sem_res != 0) {
        free(func_name);
        parse_error(p, sem_res, "Semantic error in function call");
    }

    // codegen
    if (strcmp(func_name, "print") == 0)           codegen_builtin_print(arg_types[0]);
    else if (strcmp(func_name, "readInt") == 0)    codegen_builtin_read_int();
    else if (strcmp(func_name, "readDouble") == 0) codegen_builtin_read_double();
    else if (strcmp(func_name, "readLine") == 0)   codegen_builtin_read_line();
    else                                           codegen_call_func(func_name);

    free(func_name);
    return return_type;
}

void parse_return(Parser *p) {
    expect(p, TOKEN_RETURN);

    DataType expr_type = TYPE_UNIT;
    TokenType t = curr_token(p)->type;

    if (t != TOKEN_NEWLINE && t != TOKEN_EOF) {
        expr_type = parse_expr_or_call(p);
    } else {
        codegen_push_unit();
    }

    int sem_res = sem_check_return(p->curr_func_ret_type, expr_type);
    if (sem_res != 0) {
        parse_error(p, sem_res, "Return statement error");
    }

    if (p->curr_func_ret_type == TYPE_DOUBLE && expr_type == TYPE_INT) {
        codegen_int2float_top();
    }

    codegen_func_return();
}

void parse_assign(Parser *p) {
    if (curr_token(p)->type != TOKEN_ID) {
        parse_error(p, SYNTAX_ERR_CODE, "Expected variable identifier");
    }

    char *var_name = mystrdup(curr_token(p)->value.name);
    if (!var_name) {
        parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
    }
    advance(p);

    if (!match(p, TOKEN_ASSIGN)) {
        free(var_name);
        parse_error(p, SYNTAX_ERR_CODE, "Expected '=' in assignment");
    }

    DataType expr_type = parse_expr_or_call(p);

    int sem_res = sem_check_assignment(var_name, expr_type);
    if (sem_res != 0) {
        free(var_name);
        parse_error(p, sem_res, "Semantic error in assignment");
    }

    DataType var_type = TYPE_UNIT;
    sem_get_variable_type(var_name, &var_type);
    if (var_type == TYPE_DOUBLE && expr_type == TYPE_INT) {
        codegen_int2float_top();
    }

    int scope_id = 0;
    codegen_pop_to_var(var_name, scope_id);
    free(var_name);
}

// Parses the mandatory start-of-file prolog: import ifj26.*
static void parse_first_import(Parser *p) {
    expect(p, TOKEN_IMPORT);

    Token *id = curr_token(p);
    if (id->type != TOKEN_ID || strcmp(id->value.name, "ifj26") != 0) {
        parse_error(p, SYNTAX_ERR_CODE, "Expected import ifj26.*");
    }
    advance(p); // consume "ifj26"

    expect(p, TOKEN_DOT);
    expect(p, TOKEN_MUL);
}

// Parses indented block inside func/if/while
static void parse_block(Parser *p, unsigned header_indent) {
    if (curr_token(p)->type != TOKEN_NEWLINE) {
        parse_error(p, SYNTAX_ERR_CODE, "Expected NEWLINE before block body");
    }

    unsigned block_indent = curr_token(p)->value.indent_weight;
    if (block_indent <= header_indent) {
        parse_error(p, SYNTAX_ERR_CODE, "Indent of block must be greater than header indent");
    }

    bool parsed_one = false;

    while (curr_token(p)->type != TOKEN_EOF) {
        // advance from NEWLINE to statement
        advance(p);

        if (curr_token(p)->type == TOKEN_EOF) {
            break;
        }

        parse_statement(p, block_indent);
        parsed_one = true;

        if (curr_token(p)->type == TOKEN_NEWLINE) {
            unsigned next_indent = curr_token(p)->value.indent_weight;
            if (next_indent < block_indent) {
                break; // leave NEWLINE for enclosing construct
            }
            if (next_indent > block_indent) {
                parse_error(p, SYNTAX_ERR_CODE, "Unexpected indentation increase inside block");
            }
        } else if (curr_token(p)->type != TOKEN_EOF) {
            parse_error(p, SYNTAX_ERR_CODE, "Expected NEWLINE after statement");
        }
    }

    if (!parsed_one) {
        parse_error(p, SYNTAX_ERR_CODE, "Block must have at least one statement");
    }
}

// Parses val or var declaration
static void parse_var(Parser *p) {
    bool is_mutable = (curr_token(p)->type == TOKEN_VAR);
    advance(p);

    if (curr_token(p)->type != TOKEN_ID) {
        parse_error(p, SYNTAX_ERR_CODE, "Expected variable identifier");
    }

    char *var_name = mystrdup(curr_token(p)->value.name);
    if (!var_name) {
        parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
    }
    advance(p);

    bool has_expl_type = false;
    DataType expl_type = TYPE_UNIT;
    if (match(p, TOKEN_COLON)) {
        has_expl_type = true;
        if      (match(p, TOKEN_INT))    expl_type = TYPE_INT;
        else if (match(p, TOKEN_DOUBLE)) expl_type = TYPE_DOUBLE;
        else if (match(p, TOKEN_STRING)) expl_type = TYPE_STRING;
        else if (match(p, TOKEN_UNIT))   expl_type = TYPE_UNIT;
        else {
            free(var_name);
            parse_error(p, SYNTAX_ERR_CODE, "Unknown data type");
        }
    }

    if (!match(p, TOKEN_ASSIGN)) {
        free(var_name);
        parse_error(p, SYNTAX_ERR_CODE, "Expected '=' in variable declaration");
    }

    DataType expr_type = parse_expr_or_call(p);
    DataType final_type;

    if (has_expl_type) {
        if (expl_type == expr_type) {
            final_type = expl_type;
        } else if (expl_type == TYPE_DOUBLE && expr_type == TYPE_INT) {
            final_type = TYPE_DOUBLE;
            codegen_int2float_top();
        } else {
            free(var_name);
            parse_error(p, SEM_ERR_CODE_DEF_TYPE, "val/var type differs from expected");
        }
    } else {
        if (expr_type == TYPE_BOOLEAN) {
            free(var_name);
            parse_error(p, SEM_ERR_CODE_DEF_TYPE, "Boolean cannot be stored in variable in base IFJ26");
        }
        final_type = expr_type;
    }

    SymbolData sym;
    sym.symbol_type = SYMBOL_VAR;
    sym.type = final_type;
    sym.data.var_data.is_mutable = is_mutable;

    int sem_res = sem_define_variable(var_name, sym, is_mutable);
    if (sem_res != 0) {
        free(var_name);
        parse_error(p, sem_res, "Semantic error during variable declaration");
    }

    int scope_id = 0;
    codegen_def_var(var_name, scope_id);
    codegen_pop_to_var(var_name, scope_id);
    free(var_name);
}

// Parses top-level function definition
static void parse_func(Parser *p) {
    bool is_main = false;
    if (match(p, TOKEN_MAINFUNC)) {
        is_main = true;
        if (p->was_mainfunc) {
            parse_error(p, SEM_ERR_CODE_REDEF, "Redefinition of @main function");
        }
        p->was_mainfunc = true;

        // @main may appear on its own line before def
        if (curr_token(p)->type == TOKEN_NEWLINE) {
            if (curr_token(p)->value.indent_weight != 0) {
                parse_error(p, SYNTAX_ERR_CODE, "Expected indent 0 for def after @main");
            }
            advance(p);
        }
    }

    expect(p, TOKEN_DEF);
    if (curr_token(p)->type != TOKEN_ID) {
        parse_error(p, SYNTAX_ERR_CODE, "Expected function name");
    }

    char *func_name = mystrdup(curr_token(p)->value.name);
    if (!func_name) {
        parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
    }
    advance(p);

    if (!match(p, TOKEN_PAREN_L)) {
        free(func_name);
        parse_error(p, SYNTAX_ERR_CODE, "Expected '('");
    }

    FuncParam param[MAX_PARAM_COUNT];
    unsigned param_count = 0;

    if (curr_token(p)->type != TOKEN_PAREN_R) {
        while (true) {
            if (param_count >= MAX_PARAM_COUNT) {
                free_params(param, param_count);
                free(func_name);
                parse_error(p, INTERNAL_ERR_CODE, "Too many parameters for a function");
            }

            if (curr_token(p)->type != TOKEN_ID) {
                free_params(param, param_count);
                free(func_name);
                parse_error(p, SYNTAX_ERR_CODE, "Expected parameter name");
            }

            param[param_count].name = mystrdup(curr_token(p)->value.name);
            if (!param[param_count].name) {
                free_params(param, param_count);
                free(func_name);
                parse_error(p, INTERNAL_ERR_CODE, "Memory allocation failed");
            }
            advance(p);

            if (!match(p, TOKEN_COLON)) {
                free(param[param_count].name);
                free_params(param, param_count);
                free(func_name);
                parse_error(p, SYNTAX_ERR_CODE, "Expected ':'");
            }

            if      (match(p, TOKEN_INT))    param[param_count].type = TYPE_INT;
            else if (match(p, TOKEN_DOUBLE)) param[param_count].type = TYPE_DOUBLE;
            else if (match(p, TOKEN_STRING)) param[param_count].type = TYPE_STRING;
            else if (match(p, TOKEN_UNIT))   param[param_count].type = TYPE_UNIT;
            else {
                free(param[param_count].name);
                free_params(param, param_count);
                free(func_name);
                parse_error(p, SYNTAX_ERR_CODE, "Unknown type of parameter");
            }

            param_count++;

            if (match(p, TOKEN_COMMA)) {
                if (curr_token(p)->type == TOKEN_PAREN_R) {
                    break;
                }
                continue;
            } else {
                break;
            }
        }
    }

    if (!match(p, TOKEN_PAREN_R)) {
        free_params(param, param_count);
        free(func_name);
        parse_error(p, SYNTAX_ERR_CODE, "Expected ')' after parameter list");
    }

    // optional return type
    DataType return_type = TYPE_UNIT;
    if (match(p, TOKEN_COLON)) {
        if      (match(p, TOKEN_INT))    return_type = TYPE_INT;
        else if (match(p, TOKEN_DOUBLE)) return_type = TYPE_DOUBLE;
        else if (match(p, TOKEN_STRING)) return_type = TYPE_STRING;
        else if (match(p, TOKEN_UNIT))   return_type = TYPE_UNIT;
        else {
            free_params(param, param_count);
            free(func_name);
            parse_error(p, SYNTAX_ERR_CODE, "Unknown function return type");
        }
    }

    // semantic checks for @main
    if (is_main) {
        if (param_count > 0) {
            free_params(param, param_count);
            free(func_name);
            parse_error(p, SEM_ERR_CODE_ARGS, "@main cannot have parameters");
        }
        if (return_type != TYPE_UNIT) {
            free_params(param, param_count);
            free(func_name);
            parse_error(p, SEM_ERR_CODE_DEF_TYPE, "@main must have return type Unit");
        }
    }

    DataType *param_types = NULL;
    if (param_count > 0) {
        param_types = malloc(param_count * sizeof(DataType));
        if (!param_types) {
            free_params(param, param_count);
            free(func_name);
            parse_error(p, INTERNAL_ERR_CODE, "Memory allocation for func parameters failed");
        }
        for (unsigned i = 0; i < param_count; i++) {
            param_types[i] = param[i].type;
        }
    }

    int sem_res = sem_define_function(func_name, return_type, param_count, param_types);
    free(param_types);
    if (sem_res != 0) {
        free_params(param, param_count);
        free(func_name);
        parse_error(p, sem_res, "Semantic error: function definition");
    }

    if (!match(p, TOKEN_ASSIGN)) {
        free_params(param, param_count);
        free(func_name);
        parse_error(p, SYNTAX_ERR_CODE, "Expected '=' after function header");
    }

    if (is_main) {
        if (codegen_main_start() != 0) {
            free_params(param, param_count);
            free(func_name);
            parse_error(p, INTERNAL_ERR_CODE, "Codegen main start failed");
        }
    } else {
        if (codegen_func_start(func_name) != 0) {
            free_params(param, param_count);
            free(func_name);
            parse_error(p, INTERNAL_ERR_CODE, "Codegen func start failed");
        }
    }

    sem_enter_scope();
    int scope_id = 0;

    // register parameters in local scope
    for (unsigned i = 0; i < param_count; i++) {
        SymbolData param_sym;
        param_sym.symbol_type = SYMBOL_VAR;
        param_sym.type = param[i].type;
        param_sym.data.var_data.is_mutable = false;

        int p_res = sem_define_variable(param[i].name, param_sym, false);
        if (p_res != 0) {
            free_params(param, param_count);
            free(func_name);
            parse_error(p, p_res, "Semantic error registering function parameter");
        }
    }

    for (unsigned i = param_count; i > 0; i--) {
        codegen_func_param(param[i - 1].name, scope_id);
    }

    p->curr_func_ret_type = return_type;

    if (curr_token(p)->type == TOKEN_NEWLINE) {
        parse_block(p, 0);
    } else {
        DataType expr_type = parse_expr_or_call(p);
        if (return_type != TYPE_UNIT) {
            if (return_type == TYPE_DOUBLE && expr_type == TYPE_INT) {
                codegen_int2float_top();
            } else if (return_type != expr_type) {
                free_params(param, param_count);
                free(func_name);
                parse_error(p, SEM_ERR_CODE_DEF_TYPE, "Function return type does not match");
            }
        }
    }

    sem_leave_scope();

    if (is_main) {
        codegen_main_end();
    } else {
        codegen_func_end();
    }

    free_params(param, param_count);
    free(func_name);
}

// Parses if-then-else statement
static void parse_if(Parser *p, unsigned header_indent) {
    expect(p, TOKEN_IF);

    int label_id = codegen_next_label_id();
    DataType cond_type = parse_expression(p);
    if (cond_type != TYPE_BOOLEAN) {
        parse_error(p, SEM_ERR_CODE_DEF_TYPE, "Condition of 'if' must be Boolean");
    }

    codegen_if_start(label_id);
    expect(p, TOKEN_THEN);

    sem_enter_scope();
    parse_block(p, header_indent);
    sem_leave_scope();

    codegen_if_else(label_id);

    if (curr_token(p)->type != TOKEN_NEWLINE || curr_token(p)->value.indent_weight != header_indent) {
        parse_error(p, SYNTAX_ERR_CODE, "Expected 'else' aligned with 'if'");
    }
    advance(p); // consume NEWLINE
    expect(p, TOKEN_ELSE);

    sem_enter_scope();
    parse_block(p, header_indent);
    sem_leave_scope();

    codegen_if_end(label_id);
}

// Parses while-do statement
static void parse_while(Parser *p, unsigned header_indent) {
    expect(p, TOKEN_WHILE);

    int label_id = codegen_next_label_id();
    codegen_while_header(label_id);

    DataType cond_type = parse_expression(p);
    if (cond_type != TYPE_BOOLEAN) {
        parse_error(p, SEM_ERR_CODE_DEF_TYPE, "Condition in 'while' must be Boolean");
    }

    codegen_while_body_start(label_id);
    expect(p, TOKEN_DO);

    sem_enter_scope();
    parse_block(p, header_indent);
    sem_leave_scope();

    codegen_while_end(label_id);
}

static void parse_statement(Parser *p, unsigned current_indent) {
    TokenType t = curr_token(p)->type;

    switch (t) {
        case TOKEN_VAL:
        case TOKEN_VAR:
            parse_var(p);
            break;

        case TOKEN_IF:
            parse_if(p, current_indent);
            break;

        case TOKEN_WHILE:
            parse_while(p, current_indent);
            break;

        case TOKEN_RETURN:
            parse_return(p);
            break;

        case TOKEN_ID: {
            Token *next = peek_token(p);
            if (next->type == TOKEN_ASSIGN) {
                parse_assign(p);
            } else if (next->type == TOKEN_PAREN_L) {
                parse_func_call(p);
            } else {
                parse_expression(p);
            }
            break;
        }

        case TOKEN_DEF:
        case TOKEN_MAINFUNC:
        case TOKEN_IMPORT:
        case TOKEN_ELSE:
        case TOKEN_THEN:
        case TOKEN_DO:
        case TOKEN_NEWLINE:
        case TOKEN_EOF:
            parse_error(p, SYNTAX_ERR_CODE, "Unexpected token at start of statement");
            break;

        default:
            // Standalone expression statement (including Unit literal "()")
            parse_expr_or_call(p);
            break;
    }
}

// Parses the entire source program
int parse(Lexer *lexer) {
    if (!lexer) {
        return INTERNAL_ERR_CODE;
    }

    if (codegen_init() != 0) {
        return INTERNAL_ERR_CODE;
    }

    sem_init();

    Parser p;
    p.lexer = lexer;
    p.buffer = NULL;
    p.was_mainfunc = false;
    p.curr_func_ret_type = TYPE_UNIT;

    p.buffer = token_buffer_init(TOKEN_BUFFER_INIT_CAP);
    if (!p.buffer) {
        parse_error(&p, INTERNAL_ERR_CODE, "Memory allocation failed");
    }

    load_first_token(p.lexer, p.buffer);

    // Skip any leading blank/comment lines before the prolog
    while (curr_token(&p)->type == TOKEN_NEWLINE) {
        advance(&p);
    }

    parse_first_import(&p);
    codegen_program_start();

    while (curr_token(&p)->type != TOKEN_EOF) {
        if (curr_token(&p)->type != TOKEN_NEWLINE) {
            parse_error(&p, SYNTAX_ERR_CODE, "Expected NEWLINE before top-level function");
        }
        unsigned next_indent = curr_token(&p)->value.indent_weight;
        advance(&p);

        if (curr_token(&p)->type == TOKEN_EOF) {
            break;
        }
        if (next_indent != 0) {
            parse_error(&p, SYNTAX_ERR_CODE, "Top-level function must have indent of 0");
        }

        parse_func(&p);
    }

    if (!p.was_mainfunc) {
        parse_error(&p, SEM_ERR_CODE_NO_DEF, "Semantic error: missing @main function");
    }

    codegen_flush();
    codegen_free();
    token_buffer_free(p.buffer);
    sem_dispose();

    return 0;
}

int main(void) {
    Lexer *lexer = lexer_init(stdin);
    int rc = parse(lexer);
    // TODO: call lexer_free(lexer) if your lexer module allocates heap memory
    return rc;
}