#include "codegen.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUF_CAP 1024

// Contains code string
// Works as a replacement for printf() since
//  stdount needs to say clean if an error
//  happens. This approach could theoretically
//  cause an OOM error if compiled binary is crazy
//  large or in a very tight test environment
//
// Another benifit is that a buffer makes it easier
//  to juggle instructions if needed.
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} CodeBuffer;

static CodeBuffer out_buf = {NULL, 0, 0};       // Final complete program

// func_def_buf and func_body_buf work together to avoid error 52 - redefinition
//  When inside a function and emit() is called, DEFVARs are put into the defs_buf
//  and the body is put into the body_buf, when codegen_func_end is called, these 
//  buffers are flushed like so:
//  LABEL $name
//  CREATEFRAME, PUSHFRAME
//  [contents of func_defs_buf]
//  [contents of func_body_buf]
//  POPFRAME, RETURN (or EXIT if @main)
static CodeBuffer func_defs_buf = {NULL, 0, 0};
static CodeBuffer func_body_buf = {NULL, 0, 0};
static CodeBuffer *active_buf = &out_buf;       // Where emit() currently writes
static char *current_func_name = NULL;
static int label_counter = 0;

static int buf_init(CodeBuffer *buf) {
    buf->data = malloc(INITIAL_BUF_CAP);
    if (!buf->data) return 99;
    buf->cap = INITIAL_BUF_CAP;
    buf->len = 0;
    buf->data[0] = '\0';
    return 0;
}

static void buf_free(CodeBuffer *buf) {
    free(buf->data);
    buf->data = NULL;
    buf->len = 0;
    buf->cap = 0;
}

static void buf_reset(CodeBuffer *buf) {
    if (buf->data) {
        buf->len = 0;
        buf->data[0] = '\0';
    }
}

static int buf_emit(CodeBuffer *buf, const char *fmt, ...) {
    if (!buf || !buf->data) return 99;

    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (needed < 0) return 99;

    while (buf->len + (size_t)needed + 1 > buf->cap) {
        size_t new_cap = buf->cap * 2;
        char *new_data = realloc(buf->data, new_cap);
        if (!new_data) return 99;
        buf->data = new_data;
        buf->cap = new_cap;
    }

    va_start(args, fmt);
    vsnprintf(buf->data + buf->len, (size_t)needed + 1, fmt, args);
    va_end(args);

    buf->len += (size_t)needed;
    return 0;
}

#define emit(...) buf_emit(active_buf, __VA_ARGS__)

// Outputs initialization header
int codegen_init(void) {
    label_counter = 0;
    if (buf_init(&out_buf) != 0) return 99;
    if (buf_init(&func_defs_buf) != 0) {
        buf_free(&out_buf);
        return 99;
    }
    if (buf_init(&func_body_buf) != 0) {
        buf_free(&out_buf);
        buf_free(&func_defs_buf);
        return 99;
    }
    active_buf = &out_buf;
    return emit(".IFJcode26\n");
}

// Frees buffer
void codegen_free(void) {
    buf_free(&out_buf);
    buf_free(&func_defs_buf);
    buf_free(&func_body_buf);
    active_buf = &out_buf;
}

// Outputs buffered code to stdout
void codegen_flush(void) {
    if (out_buf.data) {
        fputs(out_buf.data, stdout);
    }
}

int codegen_next_label_id(void) {
    return ++label_counter;
}

void codegen_program_start(void) {
    // Scratchpad global variables for stack tricks (like string concat or 2nd-item float cast)
    emit("DEFVAR GF@$tmp1\n");
    emit("DEFVAR GF@$tmp2\n");
    emit("JUMP $$main\n\n");
}

// Helper: prints a C string in IFJcode26 escaped format (e.g. "a b" -> "a\032b")
static void emit_escaped_string(const char *str) {
    if (!str) return;
    for (size_t i = 0; str[i] != '\0'; i++) {
        unsigned char c = (unsigned char)str[i];
        if (c <= 32 || c == '#' || c == '\\') {
            emit("\\%03d", c);
        } else {
            emit("%c", c);
        }
    }
}

void codegen_def_var(const char *var_name) {
    // ALWAYS hoist defvars in function definition buffer to avoid redefinition in loops
    buf_emit(&func_defs_buf, "DEFVAR LF@%s\n", var_name);
}

void codegen_pop_to_var(const char *var_name) {
    emit("POPS LF@%s\n", var_name);
}

void codegen_push_var(const char *var_name) {
    emit("PUSHS LF@%s\n", var_name);
}

void codegen_push_int(int val) {
    emit("PUSHS int@%d\n", val);
}

void codegen_push_double(double val) {
    // %a outputs exact hexadecimal float format, which ic26int expects!
    emit("PUSHS float@%a\n", val);
}

void codegen_push_string(const char *str) {
    emit("PUSHS string@");
    emit_escaped_string(str);
    emit("\n");
}

void codegen_push_unit(void) {
    emit("PUSHS nil@nil\n");
}

void codegen_int2float_top(void) {
    emit("INT2FLOATS\n");
}

void codegen_int2float_second(void) {
    emit("POPS GF@$tmp1\n");
    emit("INT2FLOATS\n");
    emit("PUSHS GF@$tmp1\n");
}

void codegen_unary_op(TokenType op) {
    if (op == TOKEN_SUB) {
        // Stack has [val]. We want [0 - val].
        emit("POPS GF@$tmp1\n");
        emit("PUSHS int@0\n");
        emit("PUSHS GF@$tmp1\n");
        emit("SUBS\n");
    }
}

void codegen_binary_op(TokenType op, DataType operand_type) {
    switch (op) {
        case TOKEN_ADD:
            if (operand_type == TYPE_STRING) {
                emit("POPS GF@$tmp2\n");
                emit("POPS GF@$tmp1\n");
                emit("CONCAT GF@$tmp1 GF@$tmp1 GF@$tmp2\n");
                emit("PUSHS GF@$tmp1\n");
            } else {
                emit("ADDS\n");
            }
            break;
        case TOKEN_SUB:
            emit("SUBS\n");
            break;
        case TOKEN_MUL:
            emit("MULS\n");
            break;
        case TOKEN_DIV:
            if (operand_type == TYPE_DOUBLE) {
                emit("DIVS\n");
            } else {
                emit("IDIVS\n");
            }
            break;
        case TOKEN_EQUALS:
            emit("EQS\n");
            break;
        case TOKEN_NOTEQUALS:
            emit("EQS\n");
            emit("NOTS\n");
            break;
        case TOKEN_LESS:
            emit("LTS\n");
            break;
        case TOKEN_GREATER:
            emit("GTS\n");
            break;
        case TOKEN_LESSEQUAL:
            // a <= b  <=>  !(a > b)
            emit("GTS\n");
            emit("NOTS\n");
            break;
        case TOKEN_GREATEREQUAL:
            // a >= b  <=>  !(a < b)
            emit("LTS\n");
            emit("NOTS\n");
            break;
        default:
            break;
    }
}

void codegen_if_start(int label_id) {
    emit("PUSHS bool@true\n");
    emit("JUMPIFNEQS $if_else_%d\n", label_id);
}

void codegen_if_else(int label_id) {
    emit("JUMP $if_end_%d\n", label_id);
    emit("LABEL $if_else_%d\n", label_id);
}

void codegen_if_end(int label_id) {
    emit("LABEL $if_end_%d\n", label_id);
}

void codegen_while_header(int label_id) {
    emit("LABEL $while_start_%d\n", label_id);
}

void codegen_while_body_start(int label_id) {
    emit("PUSHS bool@true\n");
    emit("JUMPIFNEQS $while_end_%d\n", label_id);
}

void codegen_while_end(int label_id) {
    emit("JUMP $while_start_%d\n", label_id);
    emit("LABEL $while_end_%d\n", label_id);
}

int codegen_main_start(void) {
    buf_reset(&func_defs_buf);
    buf_reset(&func_body_buf);
    active_buf = &func_body_buf;
    return 0;
}

void codegen_main_end(void) {
    active_buf = &out_buf;
    emit("LABEL $$main\n");
    emit("CREATEFRAME\n");
    emit("PUSHFRAME\n");
    if (func_defs_buf.len > 0) emit("%s", func_defs_buf.data);
    if (func_body_buf.len > 0) emit("%s", func_body_buf.data);
    emit("POPFRAME\n");
    emit("EXIT int@0\n\n");
}

int codegen_func_start(const char *func_name) {
    free(current_func_name);
    current_func_name = malloc(strlen(func_name) + 1);
    if (!current_func_name) return 99;
    strcpy(current_func_name, func_name);

    buf_reset(&func_defs_buf);
    buf_reset(&func_body_buf);
    active_buf = &func_body_buf;
    return 0;
}

void codegen_func_end(void) {
    active_buf = &out_buf;
    emit("LABEL $%s\n", current_func_name);
    emit("CREATEFRAME\n");
    emit("PUSHFRAME\n");
    if (func_defs_buf.len > 0) emit("%s", func_defs_buf.data);
    if (func_body_buf.len > 0) emit("%s", func_body_buf.data);
    emit("PUSHS nil@nil\n");
    emit("POPFRAME\n");
    emit("RETURN\n\n");

    free(current_func_name);
    current_func_name = NULL;
}
