#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>
#include "bst.h"
#include "token.h"

// Initializes the code internal buffer and emits the .IFJcode26 header.
// Returns 0 on success, 99 on malloc failure.
int codegen_init(void);

// Frees all internal memory used by codegen
void codegen_free(void);

// Outputs filled code buffer to stdout
void codegen_flush(void);

// Emits program start to code buffer
void codegen_program_start(void);

#endif // CODEGEN_H
