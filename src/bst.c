#include "bst.h"
#include <stdlib.h>
#include <string.h>

// Stroct for nodes in BST
struct Node {
    char *key;              // Identifier of variable
    VariantValue value;     // Variable value and data type
    
    struct Node *branch_L;  // For lower key value than parent node
    struct Node *branch_R;  // For higher key value than parent node
};

// Creates original node for BST and sets it to NULL.
void bst_init(bst_node_t **origin){
    *origin = NULL;
}

// Creates new node in BST
void bst_insert(bst_node_t **origin, char *key, VariantValue value){
    if(origin == NULL || key == NULL) return NULL; // Check if parameters are correct.

    bst_node_t **node = origin;

    // Find free space in BST.
    while(*node != NULL){
        int placement = strcmp(key, (*node)->key);

        if(placement < 0) node = &((*node)->branch_L);
        else if (placement > 0) node = &((*node)->branch_R);
        // TO DO: error if == 0
    }

    // Make space for new node
    *node = malloc(sizeof(bst_node_t));
    if(*node == NULL) return;

    (*node)->key = malloc(sizeof(char) * (strlen(key) + 1));
    if((*node)->key == NULL){
        free(*node);
        return;
    }

    // Assign values
    (*node)->value = value;
    strcpy((*node)->key, key);
    (*node)->branch_L = NULL;
    (*node)->branch_R = NULL;
}

// Returns value of key stored in BST
VariantValue *bst_search(bst_node_t *origin, char *key){
    if(origin == NULL || key == NULL) return NULL; // Check if parameters are correct.

    bst_node_t *node = origin;

    // Find and return value of key
    while(node != NULL){
        int placement = strcmp(key, node->key);

        if(placement < 0) node = node->branch_L;
        else if (placement > 0) node = node->branch_R;
        else return &(node->value);
    }

    // Return NULL if key was not found
    return NULL;
}

// Frees BST from memory
void bst_destroy(bst_node_t **origin){
    if(origin == NULL || *origin == NULL) return; // Check if origin has contents

    bst_node_t **node = origin;

    // Destroy children nodes
    bst_destroy(&((*node)->branch_L));
    bst_destroy(&((*node)->branch_R));

    // Free dynamic values if they are assigned
    if ((*origin)->value.type == TYPE_STRING) {
        free((*origin)->value.value.string_val);
    }

    // Free node from memory
    free((*node)->key);
    free(*node);
}