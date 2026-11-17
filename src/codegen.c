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
    emit("JUMP $$main\n\n");
}
