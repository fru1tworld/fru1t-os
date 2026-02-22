#pragma once
#include "kernel.h"
#include "btree.h"

// I-node 상수
#define MAX_INODE_COUNT 256
#define MAX_FILENAME_LEN 64
#define MAX_FILE_BLOCKS 12
#define DIRECT_BLOCKS 10
#define INDIRECT_BLOCK 10
#define DOUBLE_INDIRECT_BLOCK 11

// 파일 데이터용 블록 크기
#define FS_BLOCK_SIZE 512
#define MAX_BLOCKS 1024

// I-node 타입
#define INODE_TYPE_FREE 0
#define INODE_TYPE_FILE 1
#define INODE_TYPE_DIR 2

// 파일 권한
#define PERM_READ  0x4
#define PERM_WRITE 0x2
#define PERM_EXEC  0x1

// I-node 구조체 (Unix i-node와 유사)
struct inode {
    uint32_t inode_num;                          // I-node number (unique identifier)
    uint32_t type;                               // Type: file, directory, etc.
    uint32_t size;                               // File size in bytes
    uint32_t permissions;                        // 파일 권한
    uint32_t link_count;                         // Number of hard links
    uint32_t block_count;                        // Number of blocks used

    // Direct block pointers
    uint32_t direct_blocks[DIRECT_BLOCKS];       // Direct block pointers
    uint32_t indirect_block;                     // Single indirect block pointer
    uint32_t double_indirect_block;              // Double indirect block pointer

    // Metadata
    uint32_t created_time;                       // Creation timestamp
    uint32_t modified_time;                      // Last modification timestamp
    uint32_t accessed_time;                      // Last access timestamp

    int in_use;                                  // 1 if i-node is in use
};

// Directory entry structure
struct dirent {
    uint32_t inode_num;                          // I-node number
    char filename[MAX_FILENAME_LEN];             // Filename
    int in_use;                                  // 1 if entry is valid
};

// File system structure with B-Tree indexing
struct btree_filesystem {
    struct btree inode_tree;                     // B-Tree for fast i-node lookup by number
    struct btree name_tree;                      // B-Tree for filename to i-node mapping
    struct inode inodes[MAX_INODE_COUNT];        // I-node table
    uint8_t *block_storage;                      // Block storage area
    uint32_t block_bitmap[MAX_BLOCKS / 32];      // Block allocation bitmap
    uint32_t inode_bitmap[MAX_INODE_COUNT / 32]; // I-node allocation bitmap
    int total_inodes;                            // Total number of i-nodes
    int free_inodes;                             // Number of free i-nodes
    int total_blocks;                            // Total number of blocks
    int free_blocks;                             // Number of free blocks
};

// I-node operations
void inode_fs_init(struct btree_filesystem *fs);
struct inode *inode_alloc(struct btree_filesystem *fs, uint32_t type);
void inode_free(struct btree_filesystem *fs, struct inode *inode);
struct inode *inode_get(struct btree_filesystem *fs, uint32_t inode_num);
int inode_read(struct btree_filesystem *fs, struct inode *inode, void *buffer, uint32_t offset, uint32_t size);
int inode_write(struct btree_filesystem *fs, struct inode *inode, const void *data, uint32_t offset, uint32_t size);
int inode_truncate(struct btree_filesystem *fs, struct inode *inode, uint32_t new_size);

// Block operations
uint32_t block_alloc(struct btree_filesystem *fs);
void block_free(struct btree_filesystem *fs, uint32_t block_num);
uint8_t *block_get_ptr(struct btree_filesystem *fs, uint32_t block_num);
int block_is_allocated(struct btree_filesystem *fs, uint32_t block_num);

// File operations using B-Tree and i-nodes
int btree_fs_create(struct btree_filesystem *fs, const char *filename, uint32_t type);
int btree_fs_open(struct btree_filesystem *fs, const char *filename);
int btree_fs_read(struct btree_filesystem *fs, const char *filename, void *buffer, uint32_t size);
int btree_fs_write(struct btree_filesystem *fs, const char *filename, const void *data, uint32_t size);
int btree_fs_delete(struct btree_filesystem *fs, const char *filename);
void btree_fs_list(struct btree_filesystem *fs);
void btree_fs_stat(struct btree_filesystem *fs, const char *filename);

// Utility functions
uint32_t hash_string(const char *str);
void inode_print(struct inode *inode);
void fs_print_stats(struct btree_filesystem *fs);
