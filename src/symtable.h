#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>

// Data types for IFJ26
typedef enum {
    TYPE_INT,       // int
    TYPE_DOUBLE,    // float
    TYPE_STRING,    // string
    TYPE_UNIT,      // (void)
    TYPE_ANY,       // any type possible
    TYPE_BOOLEAN    // return type for relation operations
} DataType;

typedef enum {
    SYMBOL_VAR,     // when storing variable
    SYMBOL_FUNC     // when storing function
} SymbolType;

// Struct containing all data types
typedef struct {
    SymbolType symbol_type; // Says if symbol is variable or function
    DataType type;          // Stores symbol type
    
    union{
        // Variable data
        struct {
            bool is_mutable;       // Says if value is mutable(var or val)
        } var_data;
        
        // Function data
        struct {
            int param_count;       // Number of function parameters
            DataType *param_types; // Array of parameter types
        } func_data;
    } data;
} SymbolData;

typedef struct Node bst_node_t;

// Creates root node for BST and sets it to NULL.
void bst_init(bst_node_t **root);

// Creates new node in BST 
// Returns error 4 if node alredy exists
// Returns error 99 if malloc operation failed
int bst_insert(bst_node_t **root, char *key, SymbolData value);

// Returns value of key stored in BST
SymbolData *bst_search(bst_node_t *root, char *key);

// Frees BST from memory
void bst_destroy(bst_node_t **root);

#endif