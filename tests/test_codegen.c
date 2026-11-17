//compile from root

#include "../src/codegen.h"
#include <assert.h>

int main(void) {
    assert(codegen_init() == 0);
    codegen_program_start();
    codegen_flush();
    codegen_free();
    return 0;
}
