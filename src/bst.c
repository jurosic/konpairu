#include "bst.h"
#include <stdlib.h>
#include <string.h>

// Stroct for nodes in BST
struct Node {
    char *key;              // Identifier of variable
    SymbolData value;       // Symbol info
    int height;             // Hight of node in bst
    
    struct Node *branch_L;  // For lower key value than parent node
    struct Node *branch_R;  // For higher key value than parent node
};

// Creates root node for BST and sets it to NULL.
void bst_init(bst_node_t **root){
    *root = NULL;
}

int get_height(bst_node_t *node) {
    if (node == NULL) return -1; 
    return node->height;
}

int get_balance(bst_node_t *node) {
    if (node == NULL) return 0;
    return get_height(node->branch_L) - get_height(node->branch_R);
}

int get_max(int a, int b) {
    return (a > b) ? a : b;
}

void rotate_right(bst_node_t **node) {
    bst_node_t *y = *node;
    bst_node_t *x = y->branch_L;
    bst_node_t *T2 = x->branch_R;

    x->branch_R = y;
    y->branch_L = T2;

    // Recalculate hights
    y->height = get_max(get_height(y->branch_L), get_height(y->branch_R)) + 1;
    x->height = get_max(get_height(x->branch_L), get_height(x->branch_R)) + 1;

    *node = x;
}

void rotate_left(bst_node_t **node) {
    bst_node_t *x = *node;
    bst_node_t *y = x->branch_R;
    bst_node_t *T2 = y->branch_L;

    y->branch_L = x;
    x->branch_R = T2;

    // Recalculate hights
    x->height = get_max(get_height(x->branch_L), get_height(x->branch_R)) + 1;
    y->height = get_max(get_height(y->branch_L), get_height(y->branch_R)) + 1;

    *node = y;
}

// Creates new node in bst and copies assigned values
// Returns error 99 if malloc operation fails
int create_node(bst_node_t **node, char *key, SymbolData value){
    // Make space for new node
    *node = malloc(sizeof(bst_node_t));
    if(*node == NULL) return 99;

    (*node)->key = malloc(sizeof(char) * (strlen(key) + 1));
    if((*node)->key == NULL){
        free(*node);
        *node = NULL;
        return 99;
    }

    // Assign values
    (*node)->height = 0;
    strcpy((*node)->key, key);
    (*node)->branch_L = NULL;
    (*node)->branch_R = NULL;
    (*node)->value = value;

    // Copy for array of function parameters
    if (value.symbol_type == SYMBOL_FUNC && value.data.func_data.param_count != 0) {
        (*node)->value.data.func_data.param_types = malloc(sizeof(DataType) * value.data.func_data.param_count);
        if ((*node)->value.data.func_data.param_types == NULL) {
            // Malloc failed
            free((*node)->key);
            (*node)->key = NULL;

            free(*node);
            *node = NULL;

            return 99;
        } 
        
        for(int i = 0; i < value.data.func_data.param_count; i++){
            (*node)->value.data.func_data.param_types[i] = value.data.func_data.param_types[i];
        }
    }

    return 0;
}

// Creates new node in BST
// Returns error 4 if node alredy exists
// Returns error 99 if malloc operation failed
int bst_insert(bst_node_t **node, char *key, SymbolData value){
    if(key == NULL || node == NULL) return 99; // Check if parameters are correct.

    int err = 0;

    // Create node when empty space is found
    if(*node == NULL){
        return create_node(node, key, value);
    }

    if (strcmp(key, (*node)->key) < 0) {
        err = bst_insert(&(*node)->branch_L, key, value);
    } else if (strcmp(key, (*node)->key) > 0) {
        err = bst_insert(&(*node)->branch_R, key, value);
    } else {
        return 4; // Node already exists(fail)
    }

    if (err != 0) return err;

    (*node)->height = get_max(get_height((*node)->branch_L), get_height((*node)->branch_R)) + 1;

    int balance = get_balance(*node);
    int balance_L = get_balance((*node)->branch_L);
    int balance_R = get_balance((*node)->branch_R);
   
    // LL rotation
    if (balance > 1 && balance_L >= 0) rotate_right(node);
    // RR rotation
    else if (balance < -1 && balance_R <= 0) rotate_left(node);
    // LR rotation
    else if (balance > 1 && balance_L < 0) {
        rotate_left(&(*node)->branch_L);
        rotate_right(node);
    }
    // RL rotation
    else if (balance < -1 && balance_R > 0) {
        rotate_right(&(*node)->branch_R);
        rotate_left(node);
    }

    return 0;
}

// Returns value of key stored in BST
SymbolData *bst_search(bst_node_t *node, char *key){
    if(node == NULL || key == NULL) return NULL; // Check if parameters are correct.

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
void bst_destroy(bst_node_t **node){
    if(node == NULL || *node == NULL) return; // Check if root has contents

    // Destroy children nodes
    bst_destroy(&((*node)->branch_L));
    bst_destroy(&((*node)->branch_R));

    // Free array of function parameter types
    if((*node)->value.symbol_type == SYMBOL_FUNC && (*node)->value.data.func_data.param_count != 0){
        free((*node)->value.data.func_data.param_types);
        (*node)->value.data.func_data.param_types = NULL;
    }

    // Free node from memory
    free((*node)->key);
    (*node)->key = NULL;
    free(*node);
    *node = NULL;
}

