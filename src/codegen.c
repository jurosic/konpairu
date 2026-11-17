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

// CodeBuffer static global instance
static CodeBuffer out_buf = {NULL, 0, 0};

// Helper to append to out_buf
static int emit(const char *fmt, ...) {
    if (!out_buf.data) return 99;

    va_list args;
    va_start(args, fmt);
	//firstly just check how many characters the fmt string will take
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (needed < 0) return 99;

    //grow buffer if needed
    while (out_buf.len + (size_t)needed + 1 > out_buf.cap) {
        size_t new_cap = out_buf.cap * 2;
        char *new_data = realloc(out_buf.data, new_cap);
        if (!new_data) return 99;
        out_buf.data = new_data;
        out_buf.cap = new_cap;
    }

    va_start(args, fmt);
    vsnprintf(out_buf.data + out_buf.len, (size_t)needed + 1, fmt, args);
    va_end(args);

    out_buf.len += (size_t)needed;
    return 0;
}

// Outputs initialization header
int codegen_init(void) {
    out_buf.data = malloc(INITIAL_BUF_CAP);
    if (!out_buf.data) return 99;

    out_buf.cap = INITIAL_BUF_CAP;
    out_buf.len = 0;
    out_buf.data[0] = '\0';

    return emit(".IFJcode26\n");
}

// Frees buffer
void codegen_free(void) {
    free(out_buf.data);
    out_buf.data = NULL;
    out_buf.len = 0;
    out_buf.cap = 0;
}

// Outputs buffered code to stdout
void codegen_flush(void) {
    if (out_buf.data) {
        fputs(out_buf.data, stdout);
    }
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
    emit("DEFVAR LF@%s\n", var_name);
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
