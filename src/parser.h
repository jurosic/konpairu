#ifndef PARSER_H
#define PARSER_H


#include <token_buffer.h>


//helper functions for parser
void error_func(void);          //replace this func when error API is done


void advance(void);             //advances to the next token
                                //calls errorfunc if next token isTOKEN_INVALID
void expect(TokenType type);    //calls errorfunc if type != current token.type


void parse_first_import(void);  //parces the mandatory first import
                                //is NOT included in parse_statement

void parse_statement(void);     //parces whole block of code
                                //calls code gen
//add parse functions here:

//data types
void parse_int(void);           
void parse_double(void);        
void parse_string(void);
void parse_unit(void);          //EXTENSION
void parse_boolean(void);       //EXTENSION
void parse_list(void);          //EXTENSION

//declarations
void parse_val_var(void);       //also calls parse_int/double/list...
//void parse_class(void);       //EXTENTION, idk what this is
//void parse_type(void);        //EXTENTION idk what this is
//void parse_extention(void);   //EXTENTION idk what this is

//logic
void parse_expression();        //parces the expression,    useful for sem. ana.
void parse_if(void);            //parces if statemnt        useful for sem. ana.
void parse_else(void);
void parse_case(void);          //EXTENTION
//void parse_match(void);       //EXTENTION idk what this is


//looooops
void parse_while(void);         //parces while loop (careful about infinite loop)
void parse_do(void);
void parse_for(void);           //EXTENTION

//other
void parse_import(void); 
void parse_def(void);
void parse_mainfunc(void);

#endif