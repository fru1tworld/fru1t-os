#pragma once
#include "kernel.h"

/* CFS와 epoll을 위한 레드-블랙 트리 구현 */

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

/* 빈 RB 트리 초기화 */
#define RB_ROOT (struct rb_root) { NULL }

/* 트리의 가장 왼쪽(최소) 노드 가져오기 */
struct rb_node *rb_first(struct rb_root *root);

/* 중위 순회에서 다음 노드 가져오기 */
struct rb_node *rb_next(struct rb_node *node);

/* RB 트리에 노드 삽입 */
void rb_insert_color(struct rb_node *node, struct rb_root *root);

/* RB 트리에서 노드 제거 */
void rb_erase(struct rb_node *node, struct rb_root *root);

/* rb_node로부터 컨테이너 구조체를 가져오는 헬퍼 매크로 */
#define rb_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

/* 노드가 비어있는지 확인 */
#define RB_EMPTY_NODE(node) ((node)->parent == (node))

/* 노드 초기화 */
#define RB_CLEAR_NODE(node) ((node)->parent = (node))

/* 트리가 비어있는지 확인하는 헬퍼 */
static inline int rb_empty_root(struct rb_root *root) {
    return root->rb_node == NULL;
}
