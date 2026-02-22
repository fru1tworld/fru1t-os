#pragma once
#include "kernel.h"

// B-트리 차수 (노드당 최대 자식 수)
#define BTREE_ORDER 5
#define BTREE_MIN_KEYS ((BTREE_ORDER - 1) / 2)
#define BTREE_MAX_KEYS (BTREE_ORDER - 1)

// 전방 선언
struct btree_node;
struct btree;

// B-트리 노드 구조체
struct btree_node {
    int num_keys;                                // 현재 노드의 키 개수
    uint32_t keys[BTREE_MAX_KEYS];              // 키 배열 (i-node 번호)
    void *values[BTREE_MAX_KEYS];               // 값 배열 (데이터 포인터)
    struct btree_node *children[BTREE_ORDER];    // 자식 포인터 배열
    struct btree_node *parent;                   // 부모 노드 포인터
    int is_leaf;                                 // 리프 노드면 1, 내부 노드면 0
};

// B-트리 구조체
struct btree {
    struct btree_node *root;                     // 루트 노드
    int height;                                  // 트리 높이
    int num_nodes;                               // 총 노드 개수
};

// B-트리 연산
void btree_init(struct btree *tree);
struct btree_node *btree_node_create(int is_leaf);
void *btree_search(struct btree *tree, uint32_t key);
int btree_insert(struct btree *tree, uint32_t key, void *value);
int btree_delete(struct btree *tree, uint32_t key);
void btree_traverse(struct btree *tree, void (*callback)(uint32_t key, void *value));
void btree_destroy(struct btree *tree);

// 헬퍼 함수들
void btree_split_child(struct btree_node *parent, int index, struct btree_node *child);
void btree_insert_non_full(struct btree_node *node, uint32_t key, void *value);
void btree_merge_children(struct btree_node *parent, int index);
void btree_print(struct btree *tree);
