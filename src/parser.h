#ifndef PARSER_H
#define PARSER_H


#include "lexer.h"

//returns 0 if succcesful
//errors:
//  1       = lexical
//  2       = in syntax
//  3       = missing var/val/func def or no @main
//  4       = val/var/func is redifined
//  5       = arguments count/type 
//  6       = val/var/func/ret type 
//  7       = assign value to const
//  10      = miscellaneous
//  99      = compiler error (mastly mem alloc) 
int parse(Lexer *lexer);



#endif