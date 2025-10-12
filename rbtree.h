#pragma once
#include "kernel.h"

/* Red-Black Tree implementation for CFS and epoll */

#define RB_RED   0
#define RB_BLACK 1

struct rb_node {
    struct rb_node *parent;
    struct rb_node *left;
    struct rb_node *right;
    int color;
};

struct rb_root {
    struct rb_node *rb_node;
};

/* Initialize an empty RB tree */
#define RB_ROOT (struct rb_root) { NULL }

/* Get the leftmost (minimum) node in the tree */
struct rb_node *rb_first(struct rb_root *root);

/* Get the next node in in-order traversal */
struct rb_node *rb_next(struct rb_node *node);

/* Insert a node into the RB tree */
void rb_insert_color(struct rb_node *node, struct rb_root *root);

/* Remove a node from the RB tree */
void rb_erase(struct rb_node *node, struct rb_root *root);

/* Helper macros to get container structure from rb_node */
#define rb_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

/* Check if node is empty */
#define RB_EMPTY_NODE(node) ((node)->parent == (node))

/* Initialize a node */
#define RB_CLEAR_NODE(node) ((node)->parent = (node))

/* Helper to check if tree is empty */
static inline int rb_empty_root(struct rb_root *root) {
    return root->rb_node == NULL;
}
