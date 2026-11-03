#ifndef BST_H
#define BST_H

#include <stdbool.h>

// Data types for IFJ26
typedef enum {
    TYPE_INT,       // int
    TYPE_DOUBLE,    // float
    TYPE_STRING,    // string
    TYPE_UNIT,      // (void)
    TYPE_BOOLEAN    // EXTENSION
} DataType;

// Struct containing all data types
typedef struct {
    DataType type;      // Stores type in which value is stored
    bool is_mutable;        // Says if value is mutable(var or val)
    
    union {
        int int_val;           // For integer literals
        double double_val;     // For double literals
        char *string_val;      // For string literals
        bool bool_val;         // For boolean literals
    } value;
} VariantValue;

typedef struct Node bst_node_t;

// Creates root node for BST and sets it to NULL.
void bst_init(bst_node_t **root);

// Creates new node in BST 
int bst_insert(bst_node_t **root, char *key, VariantValue value);

// Returns value of key stored in BST
VariantValue *bst_search(bst_node_t *root, char *key);

// Frees BST from memory
void bst_destroy(bst_node_t **root);

#endif