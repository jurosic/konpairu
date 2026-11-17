#ifndef PARSER_H
#define PARSER_H


#include "lexer.h"

//returns 0 if succcesful error:
//1 = lexer
//2 = syntax
//3-10 = semant
int parse(Lexer *lexer);



#endif PARSER_H