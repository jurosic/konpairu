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
	emit("DEFVAR GF@$tmp1\n");
	emit("DEFVAR GF@$tmp2\n");
	emit("DEFVAR GF@$tmp3\n");
    emit("JUMP $$main\n\n");

    // Reusable built-in subroutine: subStr(s: String, from: Int, to: Int): String
    emit("LABEL $$builtin_substr\n");
    emit("CREATEFRAME\n");
    emit("PUSHFRAME\n");
    emit("DEFVAR LF@to\n");
    emit("DEFVAR LF@from\n");
    emit("DEFVAR LF@s\n");
    emit("DEFVAR LF@len\n");
    emit("DEFVAR LF@res\n");
    emit("DEFVAR LF@ch\n");
    emit("DEFVAR LF@cond\n");
    emit("POPS LF@to\n");
    emit("POPS LF@from\n");
    emit("POPS LF@s\n");
    emit("MOVE LF@res string@\n");
    emit("STRLEN LF@len LF@s\n");
    // Bounds checks: from < 0 || from >= len || to < from || to > len -> return ""
    emit("LT LF@cond LF@from int@0\n");
    emit("JUMPIFEQ $$substr_end LF@cond bool@true\n");
    emit("LT LF@cond LF@from LF@len\n");
    emit("JUMPIFNEQ $$substr_end LF@cond bool@true\n");
    emit("LT LF@cond LF@to LF@from\n");
    emit("JUMPIFEQ $$substr_end LF@cond bool@true\n");
    emit("GT LF@cond LF@to LF@len\n");
    emit("JUMPIFEQ $$substr_end LF@cond bool@true\n");
    // Loop from..to-1
    emit("LABEL $$substr_loop\n");
    emit("LT LF@cond LF@from LF@to\n");
    emit("JUMPIFNEQ $$substr_end LF@cond bool@true\n");
    emit("GETCHAR LF@ch LF@s LF@from\n");
    emit("CONCAT LF@res LF@res LF@ch\n");
    emit("ADD LF@from LF@from int@1\n");
    emit("JUMP $$substr_loop\n");
    emit("LABEL $$substr_end\n");
    emit("PUSHS LF@res\n");
    emit("POPFRAME\n");
    emit("RETURN\n\n");
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

void codegen_def_var(const char *var_name, int scope_id) {
    buf_emit(&func_defs_buf, "DEFVAR LF@%s$%d\n", var_name, scope_id);
}

void codegen_pop_to_var(const char *var_name, int scope_id) {
    emit("POPS LF@%s$%d\n", var_name, scope_id);
}

void codegen_push_var(const char *var_name, int scope_id) {
    emit("PUSHS LF@%s$%d\n", var_name, scope_id);
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

void codegen_push_bool(bool val) {
	emit("PUSHS bool@%s\n", val ? "true" : "false");
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

void codegen_builtin_to_str(DataType arg_type) {
    switch (arg_type) {
        case TYPE_INT:
            emit("INT2STRS\n");
            break;
        case TYPE_DOUBLE:
            emit("FLOAT2STRS\n");
            break;
        case TYPE_UNIT:
            emit("POPS GF@$tmp1\n");
            emit("PUSHS string@()\n");
            break;
        case TYPE_STRING:
        default:
            // Already a String on top of the stack
            break;
    }
}

void codegen_builtin_to_int(void) {
    emit("FLOAT2INTS\n");
}

void codegen_builtin_str_length(void) {
    emit("POPS GF@$tmp1\n");
    emit("STRLEN GF@$tmp1 GF@$tmp1\n");
    emit("PUSHS GF@$tmp1\n");
}

void codegen_builtin_chr(void) {
    emit("INT2CHARS\n");
}

void codegen_builtin_ord(void) {
    int id = codegen_next_label_id();
    emit("POPS GF@$tmp2\n"); // idx
    emit("POPS GF@$tmp1\n"); // s

    // 1. Check idx < 0
    emit("LT GF@$tmp3 GF@$tmp2 int@0\n");
    emit("JUMPIFEQ $ord_invalid_%d GF@$tmp3 bool@true\n", id);

    // 2. Check idx < strlen(s)
    emit("STRLEN GF@$tmp3 GF@$tmp1\n");
    emit("LT GF@$tmp3 GF@$tmp2 GF@$tmp3\n");
    emit("JUMPIFNEQ $ord_invalid_%d GF@$tmp3 bool@true\n", id);

    // 3. Valid
    emit("STRI2INT GF@$tmp1 GF@$tmp1 GF@$tmp2\n");
    emit("PUSHS GF@$tmp1\n");
    emit("JUMP $ord_end_%d\n", id);

    emit("LABEL $ord_invalid_%d\n", id);
    emit("PUSHS int@0\n");
    emit("LABEL $ord_end_%d\n", id);
}

void codegen_func_param(const char *param_name, int scope_id) {
    codegen_def_var(param_name, scope_id);
    emit("POPS LF@%s$%d\n", param_name, scope_id);
}

void codegen_call_func(const char *func_name) {
    emit("CALL $%s\n", func_name);
}

void codegen_func_return(void) {
	if (current_func_name == NULL) {
		emit("POPS GF@$tmp1\n");
		emit("POPFRAME\n");
		emit("EXIT int@0\n");
	} else {
		emit("POPFRAME\n");
		emit("RETURN\n");
	}
}

void codegen_discard_stack_top(void) {
    emit("POPS GF@$tmp1\n");
}

void codegen_builtin_print(DataType arg_type) {
    emit("POPS GF@$tmp1\n");
    if (arg_type == TYPE_UNIT) {
        emit("WRITE string@()\n");
    } else {
        emit("WRITE GF@$tmp1\n");
    }
	emit("PUSHS nil@nil\n");
}

void codegen_builtin_read_line(void) {
    emit("READ GF@$tmp1 string\n");
    emit("PUSHS GF@$tmp1\n");
}

void codegen_builtin_read_int(void) {
    int id = codegen_next_label_id();
    emit("READ GF@$tmp1 int\n");
    emit("TYPE GF@$tmp2 GF@$tmp1\n");
    emit("JUMPIFNEQ $readint_ok_%d GF@$tmp2 string@nil\n", id);
    emit("MOVE GF@$tmp1 int@0\n");
    emit("LABEL $readint_ok_%d\n", id);
    emit("PUSHS GF@$tmp1\n");
}

void codegen_builtin_read_double(void) {
    int id = codegen_next_label_id();
    emit("READ GF@$tmp1 float\n");
    emit("TYPE GF@$tmp2 GF@$tmp1\n");
    emit("JUMPIFNEQ $readdbl_ok_%d GF@$tmp2 string@nil\n", id);
    emit("MOVE GF@$tmp1 float@0x0p+0\n");
    emit("LABEL $readdbl_ok_%d\n", id);
    emit("PUSHS GF@$tmp1\n");
}

void codegen_builtin_substr(void) {
    emit("CALL $$builtin_substr\n");
}

void codegen_builtin_strcmp(void) {
	int id = codegen_next_label_id();
	emit("POPS GF@$tmp2\n");
	emit("POPS GF@$tmp1\n");
	emit("LT GF@$tmp3 GF@$tmp1 GF@$tmp2\n");
	emit("JUMPIFEQ $strcmp_lt_%d GF@$tmp3 bool@true\n", id);
	emit("EQ GF@$tmp3 GF@$tmp1 GF@$tmp2\n");
	emit("JUMPIFEQ $strcmp_eq_%d GF@$tmp3 bool@true\n", id);
	emit("PUSHS int@1\n");
	emit("JUMP $strcmp_end_%d\n", id);
	emit("LABEL $strcmp_lt_%d\n", id);
	emit("PUSHS int@-1\n");
	emit("JUMP $strcmp_end_%d\n", id);
	emit("LABEL $strcmp_eq_%d\n", id);
	emit("PUSHS int@0\n");
	emit("LABEL $strcmp_end_%d\n", id);
}
