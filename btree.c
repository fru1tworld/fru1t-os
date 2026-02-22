#include "btree.h"
#include "kernel.h"

// B-트리 초기화
void btree_init(struct btree *tree) {
    tree->root = NULL;
    tree->height = 0;
    tree->num_nodes = 0;
}

// 새 B-트리 노드 생성
struct btree_node *btree_node_create(int is_leaf) {
    struct btree_node *node = (struct btree_node *)kmalloc(sizeof(struct btree_node));
    if (!node) {
        return NULL;
    }

    node->num_keys = 0;
    node->is_leaf = is_leaf;
    node->parent = NULL;

    for (int i = 0; i < BTREE_MAX_KEYS; i++) {
        node->keys[i] = 0;
        node->values[i] = NULL;
    }

    for (int i = 0; i < BTREE_ORDER; i++) {
        node->children[i] = NULL;
    }

    return node;
}

// B-트리에서 키 검색
void *btree_search(struct btree *tree, uint32_t key) {
    if (tree->root == NULL) {
        return NULL;
    }

    struct btree_node *node = tree->root;

    while (node != NULL) {
        int i = 0;

        // 검색 키보다 크거나 같은 첫 번째 키 찾기
        while (i < node->num_keys && key > node->keys[i]) {
            i++;
        }

        // 키를 찾았는지 확인
        if (i < node->num_keys && key == node->keys[i]) {
            return node->values[i];
        }

        // 리프 노드면 키가 존재하지 않음
        if (node->is_leaf) {
            return NULL;
        }

        // 적절한 자식으로 이동
        node = node->children[i];
    }

    return NULL;
}

// 가득 찬 자식 노드 분할
void btree_split_child(struct btree_node *parent, int index, struct btree_node *child) {
    struct btree_node *new_node = btree_node_create(child->is_leaf);
    if (!new_node) {
        return;
    }

    new_node->num_keys = BTREE_MIN_KEYS;

    // 키와 값의 후반부를 새 노드로 복사
    for (int i = 0; i < BTREE_MIN_KEYS; i++) {
        new_node->keys[i] = child->keys[i + BTREE_MIN_KEYS + 1];
        new_node->values[i] = child->values[i + BTREE_MIN_KEYS + 1];
    }

    // 리프가 아니면 자식 포인터 복사
    if (!child->is_leaf) {
        for (int i = 0; i < BTREE_MIN_KEYS + 1; i++) {
            new_node->children[i] = child->children[i + BTREE_MIN_KEYS + 1];
            if (new_node->children[i]) {
                new_node->children[i]->parent = new_node;
            }
        }
    }

    child->num_keys = BTREE_MIN_KEYS;

    // Shift parent's children to make room for new node
    for (int i = parent->num_keys; i > index; i--) {
        parent->children[i + 1] = parent->children[i];
    }
    parent->children[index + 1] = new_node;
    new_node->parent = parent;

    // Move middle key up to parent
    for (int i = parent->num_keys - 1; i >= index; i--) {
        parent->keys[i + 1] = parent->keys[i];
        parent->values[i + 1] = parent->values[i];
    }
    parent->keys[index] = child->keys[BTREE_MIN_KEYS];
    parent->values[index] = child->values[BTREE_MIN_KEYS];
    parent->num_keys++;
}

// 가득 차지 않은 노드에 키-값 쌍 삽입
void btree_insert_non_full(struct btree_node *node, uint32_t key, void *value) {
    int i = node->num_keys - 1;

    if (node->is_leaf) {
        // Insert into leaf node
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }

        node->keys[i + 1] = key;
        node->values[i + 1] = value;
        node->num_keys++;
    } else {
        // Find child to insert into
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;

        // Check if child is full
        if (node->children[i]->num_keys == BTREE_MAX_KEYS) {
            btree_split_child(node, i, node->children[i]);

            if (key > node->keys[i]) {
                i++;
            }
        }

        btree_insert_non_full(node->children[i], key, value);
    }
}

// B-트리에 키-값 쌍 삽입
int btree_insert(struct btree *tree, uint32_t key, void *value) {
    // Check if key already exists
    if (btree_search(tree, key) != NULL) {
        return -1; // Key already exists
    }

    // If tree is empty, create root
    if (tree->root == NULL) {
        tree->root = btree_node_create(1);
        if (!tree->root) {
            return -1;
        }
        tree->root->keys[0] = key;
        tree->root->values[0] = value;
        tree->root->num_keys = 1;
        tree->num_nodes = 1;
        tree->height = 1;
        return 0;
    }

    // If root is full, split it
    if (tree->root->num_keys == BTREE_MAX_KEYS) {
        struct btree_node *new_root = btree_node_create(0);
        if (!new_root) {
            return -1;
        }

        new_root->children[0] = tree->root;
        tree->root->parent = new_root;
        btree_split_child(new_root, 0, tree->root);
        tree->root = new_root;
        tree->height++;
    }

    btree_insert_non_full(tree->root, key, value);
    tree->num_nodes++;
    return 0;
}

// Helper function to find key index in node
static int find_key_index(struct btree_node *node, uint32_t key) {
    int i = 0;
    while (i < node->num_keys && node->keys[i] < key) {
        i++;
    }
    return i;
}

// Get predecessor key from subtree
static void get_predecessor(struct btree_node *node, int idx, uint32_t *key, void **value) {
    struct btree_node *curr = node->children[idx];
    while (!curr->is_leaf) {
        curr = curr->children[curr->num_keys];
    }
    *key = curr->keys[curr->num_keys - 1];
    *value = curr->values[curr->num_keys - 1];
}

// Get successor key from subtree
static void get_successor(struct btree_node *node, int idx, uint32_t *key, void **value) {
    struct btree_node *curr = node->children[idx + 1];
    while (!curr->is_leaf) {
        curr = curr->children[0];
    }
    *key = curr->keys[0];
    *value = curr->values[0];
}

// Merge child with sibling
void btree_merge_children(struct btree_node *parent, int index) {
    struct btree_node *child = parent->children[index];
    struct btree_node *sibling = parent->children[index + 1];

    // Pull key from parent and merge with right sibling
    child->keys[BTREE_MIN_KEYS] = parent->keys[index];
    child->values[BTREE_MIN_KEYS] = parent->values[index];

    // Copy keys from sibling
    for (int i = 0; i < sibling->num_keys; i++) {
        child->keys[i + BTREE_MIN_KEYS + 1] = sibling->keys[i];
        child->values[i + BTREE_MIN_KEYS + 1] = sibling->values[i];
    }

    // Copy child pointers if not leaf
    if (!child->is_leaf) {
        for (int i = 0; i <= sibling->num_keys; i++) {
            child->children[i + BTREE_MIN_KEYS + 1] = sibling->children[i];
            if (child->children[i + BTREE_MIN_KEYS + 1]) {
                child->children[i + BTREE_MIN_KEYS + 1]->parent = child;
            }
        }
    }

    child->num_keys += sibling->num_keys + 1;

    // Move keys in parent
    for (int i = index + 1; i < parent->num_keys; i++) {
        parent->keys[i - 1] = parent->keys[i];
        parent->values[i - 1] = parent->values[i];
    }

    // Move child pointers in parent
    for (int i = index + 2; i <= parent->num_keys; i++) {
        parent->children[i - 1] = parent->children[i];
    }

    parent->num_keys--;
    kfree(sibling);
}

// Delete helper for non-leaf nodes
static void delete_internal_node(struct btree *tree, struct btree_node *node, uint32_t key);

// Delete helper for leaf nodes
static void delete_leaf_node(struct btree_node *node, uint32_t key) {
    int i = find_key_index(node, key);

    if (i < node->num_keys && node->keys[i] == key) {
        // Shift keys and values
        for (int j = i; j < node->num_keys - 1; j++) {
            node->keys[j] = node->keys[j + 1];
            node->values[j] = node->values[j + 1];
        }
        node->num_keys--;
    }
}

// B-트리에서 키 삭제
int btree_delete(struct btree *tree, uint32_t key) {
    if (tree->root == NULL) {
        return -1;
    }

    // Simple implementation: find and remove from leaf
    struct btree_node *node = tree->root;

    while (node != NULL) {
        int i = find_key_index(node, key);

        if (i < node->num_keys && node->keys[i] == key) {
            if (node->is_leaf) {
                delete_leaf_node(node, key);
                tree->num_nodes--;
                return 0;
            }
            // For internal nodes, replace with predecessor/successor
            // Simplified: just mark as not implemented for now
            return -1;
        }

        if (node->is_leaf) {
            return -1; // Key not found
        }

        node = node->children[i];
    }

    return -1;
}

// B-트리 중위 순회
static void traverse_recursive(struct btree_node *node, void (*callback)(uint32_t key, void *value)) {
    if (node == NULL) {
        return;
    }

    int i;
    for (i = 0; i < node->num_keys; i++) {
        if (!node->is_leaf) {
            traverse_recursive(node->children[i], callback);
        }
        callback(node->keys[i], node->values[i]);
    }

    if (!node->is_leaf) {
        traverse_recursive(node->children[i], callback);
    }
}

void btree_traverse(struct btree *tree, void (*callback)(uint32_t key, void *value)) {
    traverse_recursive(tree->root, callback);
}

// B-트리 파괴
static void destroy_recursive(struct btree_node *node) {
    if (node == NULL) {
        return;
    }

    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            destroy_recursive(node->children[i]);
        }
    }

    kfree(node);
}

void btree_destroy(struct btree *tree) {
    destroy_recursive(tree->root);
    tree->root = NULL;
    tree->height = 0;
    tree->num_nodes = 0;
}

// B-트리 구조 출력
static void print_recursive(struct btree_node *node, int level) {
    if (node == NULL) {
        return;
    }

    printf("Level %d: ", level);
    for (int i = 0; i < node->num_keys; i++) {
        printf("%u ", node->keys[i]);
    }
    printf("\n");

    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            print_recursive(node->children[i], level + 1);
        }
    }
}

void btree_print(struct btree *tree) {
    printf("B-Tree (height=%d, nodes=%d):\n", tree->height, tree->num_nodes);
    print_recursive(tree->root, 0);
}
