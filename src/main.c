#include "lexer.h"
#include "parser.h"

int main(){
    Lexer *lexer = lexer_init(stdin);
    int res = parse(lexer);
    lexer_free(lexer);
    return res;
}