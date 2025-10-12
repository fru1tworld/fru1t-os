#include "rbtree.h"

/* 레드-블랙 트리 구현 */

static inline void rb_set_parent(struct rb_node *node, struct rb_node *parent) {
    node->parent = parent;
}

static inline void rb_set_color(struct rb_node *node, int color) {
    node->color = color;
}

static inline struct rb_node *rb_parent(struct rb_node *node) {
    return node->parent;
}

static inline int rb_color(struct rb_node *node) {
    return node ? node->color : RB_BLACK;
}

static inline int rb_is_red(struct rb_node *node) {
    return node && node->color == RB_RED;
}

static inline int rb_is_black(struct rb_node *node) {
    return !node || node->color == RB_BLACK;
}

/* 왼쪽 회전 */
static void rb_rotate_left(struct rb_node *node, struct rb_root *root) {
    struct rb_node *right = node->right;
    struct rb_node *parent = rb_parent(node);

    node->right = right->left;
    if (right->left)
        rb_set_parent(right->left, node);

    right->left = node;
    rb_set_parent(right, parent);

    if (parent) {
        if (node == parent->left)
            parent->left = right;
        else
            parent->right = right;
    } else {
        root->rb_node = right;
    }
    rb_set_parent(node, right);
}

/* 오른쪽 회전 */
static void rb_rotate_right(struct rb_node *node, struct rb_root *root) {
    struct rb_node *left = node->left;
    struct rb_node *parent = rb_parent(node);

    node->left = left->right;
    if (left->right)
        rb_set_parent(left->right, node);

    left->right = node;
    rb_set_parent(left, parent);

    if (parent) {
        if (node == parent->right)
            parent->right = left;
        else
            parent->left = left;
    } else {
        root->rb_node = left;
    }
    rb_set_parent(node, left);
}

/* 삽입 및 재균형 */
void rb_insert_color(struct rb_node *node, struct rb_root *root) {
    struct rb_node *parent, *gparent;

    while ((parent = rb_parent(node)) && rb_is_red(parent)) {
        gparent = rb_parent(parent);

        if (parent == gparent->left) {
            struct rb_node *uncle = gparent->right;

            if (uncle && rb_is_red(uncle)) {
                /* 경우 1: 삼촌이 빨간색 */
                rb_set_color(uncle, RB_BLACK);
                rb_set_color(parent, RB_BLACK);
                rb_set_color(gparent, RB_RED);
                node = gparent;
                continue;
            }

            if (parent->right == node) {
                /* 경우 2: 노드가 오른쪽 자식 */
                struct rb_node *tmp;
                rb_rotate_left(parent, root);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            /* 경우 3: 노드가 왼쪽 자식 */
            rb_set_color(parent, RB_BLACK);
            rb_set_color(gparent, RB_RED);
            rb_rotate_right(gparent, root);
        } else {
            struct rb_node *uncle = gparent->left;

            if (uncle && rb_is_red(uncle)) {
                /* 경우 1: 삼촌이 빨간색 */
                rb_set_color(uncle, RB_BLACK);
                rb_set_color(parent, RB_BLACK);
                rb_set_color(gparent, RB_RED);
                node = gparent;
                continue;
            }

            if (parent->left == node) {
                /* 경우 2: 노드가 왼쪽 자식 */
                struct rb_node *tmp;
                rb_rotate_right(parent, root);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            /* 경우 3: 노드가 오른쪽 자식 */
            rb_set_color(parent, RB_BLACK);
            rb_set_color(gparent, RB_RED);
            rb_rotate_left(gparent, root);
        }
    }

    rb_set_color(root->rb_node, RB_BLACK);
}

/* 가장 왼쪽 노드(최소값) 가져오기 */
struct rb_node *rb_first(struct rb_root *root) {
    struct rb_node *node = root->rb_node;

    if (!node)
        return NULL;

    while (node->left)
        node = node->left;

    return node;
}

/* 중위 순회에서 다음 노드 가져오기 */
struct rb_node *rb_next(struct rb_node *node) {
    struct rb_node *parent;

    if (!node)
        return NULL;

    /* 오른쪽 서브트리가 존재하면, 오른쪽 서브트리의 가장 왼쪽 노드 반환 */
    if (node->right) {
        node = node->right;
        while (node->left)
            node = node->left;
        return node;
    }

    /* 부모의 왼쪽 자식인 노드를 찾을 때까지 위로 이동 */
    while ((parent = rb_parent(node)) && node == parent->right)
        node = parent;

    return parent;
}

/* 삭제 후 재균형을 위한 헬퍼 */
static void rb_erase_color(struct rb_node *node, struct rb_node *parent,
                          struct rb_root *root) {
    struct rb_node *sibling;

    while ((!node || rb_is_black(node)) && node != root->rb_node) {
        if (parent->left == node) {
            sibling = parent->right;

            if (rb_is_red(sibling)) {
                rb_set_color(sibling, RB_BLACK);
                rb_set_color(parent, RB_RED);
                rb_rotate_left(parent, root);
                sibling = parent->right;
            }

            if ((!sibling->left || rb_is_black(sibling->left)) &&
                (!sibling->right || rb_is_black(sibling->right))) {
                rb_set_color(sibling, RB_RED);
                node = parent;
                parent = rb_parent(node);
            } else {
                if (!sibling->right || rb_is_black(sibling->right)) {
                    rb_set_color(sibling->left, RB_BLACK);
                    rb_set_color(sibling, RB_RED);
                    rb_rotate_right(sibling, root);
                    sibling = parent->right;
                }

                rb_set_color(sibling, rb_color(parent));
                rb_set_color(parent, RB_BLACK);
                rb_set_color(sibling->right, RB_BLACK);
                rb_rotate_left(parent, root);
                node = root->rb_node;
                break;
            }
        } else {
            sibling = parent->left;

            if (rb_is_red(sibling)) {
                rb_set_color(sibling, RB_BLACK);
                rb_set_color(parent, RB_RED);
                rb_rotate_right(parent, root);
                sibling = parent->left;
            }

            if ((!sibling->left || rb_is_black(sibling->left)) &&
                (!sibling->right || rb_is_black(sibling->right))) {
                rb_set_color(sibling, RB_RED);
                node = parent;
                parent = rb_parent(node);
            } else {
                if (!sibling->left || rb_is_black(sibling->left)) {
                    rb_set_color(sibling->right, RB_BLACK);
                    rb_set_color(sibling, RB_RED);
                    rb_rotate_left(sibling, root);
                    sibling = parent->left;
                }

                rb_set_color(sibling, rb_color(parent));
                rb_set_color(parent, RB_BLACK);
                rb_set_color(sibling->left, RB_BLACK);
                rb_rotate_right(parent, root);
                node = root->rb_node;
                break;
            }
        }
    }

    if (node)
        rb_set_color(node, RB_BLACK);
}

/* 트리에서 노드 제거 */
void rb_erase(struct rb_node *node, struct rb_root *root) {
    struct rb_node *child, *parent;
    int color;

    if (!node->left) {
        child = node->right;
    } else if (!node->right) {
        child = node->left;
    } else {
        /* 노드에 자식이 두 개 있으면, 후속자 찾기 */
        struct rb_node *old = node;
        node = node->right;

        while (node->left)
            node = node->left;

        child = node->right;
        parent = rb_parent(node);
        color = rb_color(node);

        if (child)
            rb_set_parent(child, parent);

        if (parent) {
            if (parent->left == node)
                parent->left = child;
            else
                parent->right = child;
        } else {
            root->rb_node = child;
        }

        if (rb_parent(node) == old)
            parent = node;

        node->parent = old->parent;
        node->color = old->color;
        node->right = old->right;
        node->left = old->left;

        if (rb_parent(old)) {
            if (rb_parent(old)->left == old)
                rb_parent(old)->left = node;
            else
                rb_parent(old)->right = node;
        } else {
            root->rb_node = node;
        }

        rb_set_parent(old->left, node);
        if (old->right)
            rb_set_parent(old->right, node);

        goto color_fixup;
    }

    parent = rb_parent(node);
    color = rb_color(node);

    if (child)
        rb_set_parent(child, parent);

    if (parent) {
        if (parent->left == node)
            parent->left = child;
        else
            parent->right = child;
    } else {
        root->rb_node = child;
    }

color_fixup:
    if (color == RB_BLACK)
        rb_erase_color(child, parent, root);
}
