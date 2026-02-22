#include "inode.h"
#include "btree.h"
#include "kernel.h"

// 파일명 검색을 위한 문자열 해싱 함수
uint32_t hash_string(const char *str) {
    uint32_t hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash;
}

// 문자열 비교 헬퍼
static int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// 문자열 복사 헬퍼
static void strcpy(char *dst, const char *src) {
    while ((*dst++ = *src++));
}

// 문자열 길이 헬퍼
static size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

// Initialize bitmap
static void bitmap_init(uint32_t *bitmap, int size) {
    for (int i = 0; i < size; i++) {
        bitmap[i] = 0;
    }
}

// Set bit in bitmap
static void bitmap_set(uint32_t *bitmap, int bit) {
    bitmap[bit / 32] |= (1 << (bit % 32));
}

// Clear bit in bitmap
static void bitmap_clear(uint32_t *bitmap, int bit) {
    bitmap[bit / 32] &= ~(1 << (bit % 32));
}

// Test bit in bitmap
static int bitmap_test(uint32_t *bitmap, int bit) {
    return (bitmap[bit / 32] & (1 << (bit % 32))) != 0;
}

// Find first free bit in bitmap
static int bitmap_find_free(uint32_t *bitmap, int max_bits) {
    for (int i = 0; i < max_bits; i++) {
        if (!bitmap_test(bitmap, i)) {
            return i;
        }
    }
    return -1;
}

// 파일시스템 초기화
void inode_fs_init(struct btree_filesystem *fs) {
    // Initialize B-Trees
    btree_init(&fs->inode_tree);
    btree_init(&fs->name_tree);

    // Allocate block storage
    fs->block_storage = (uint8_t *)kmalloc(FS_BLOCK_SIZE * MAX_BLOCKS);
    if (!fs->block_storage) {
        PANIC("Failed to allocate block storage");
    }

    // Initialize bitmaps
    bitmap_init(fs->block_bitmap, MAX_BLOCKS / 32);
    bitmap_init(fs->inode_bitmap, MAX_INODE_COUNT / 32);

    // Initialize i-node table
    for (int i = 0; i < MAX_INODE_COUNT; i++) {
        fs->inodes[i].in_use = 0;
        fs->inodes[i].inode_num = i;
    }

    fs->total_inodes = MAX_INODE_COUNT;
    fs->free_inodes = MAX_INODE_COUNT;
    fs->total_blocks = MAX_BLOCKS;
    fs->free_blocks = MAX_BLOCKS;

    printf("B-Tree filesystem initialized: %d inodes, %d blocks\n",
           MAX_INODE_COUNT, MAX_BLOCKS);
}

// Allocate an i-node
struct inode *inode_alloc(struct btree_filesystem *fs, uint32_t type) {
    int inode_num = bitmap_find_free(fs->inode_bitmap, MAX_INODE_COUNT);
    if (inode_num < 0) {
        printf("Error: No free i-nodes available\n");
        return NULL;
    }

    struct inode *inode = &fs->inodes[inode_num];
    bitmap_set(fs->inode_bitmap, inode_num);

    // Initialize i-node
    inode->inode_num = inode_num;
    inode->type = type;
    inode->size = 0;
    inode->permissions = PERM_READ | PERM_WRITE;
    inode->link_count = 1;
    inode->block_count = 0;
    inode->in_use = 1;

    for (int i = 0; i < DIRECT_BLOCKS; i++) {
        inode->direct_blocks[i] = 0;
    }
    inode->indirect_block = 0;
    inode->double_indirect_block = 0;

    // Add to B-Tree
    btree_insert(&fs->inode_tree, inode_num, inode);

    fs->free_inodes--;

    return inode;
}

// Free an i-node
void inode_free(struct btree_filesystem *fs, struct inode *inode) {
    if (!inode || !inode->in_use) {
        return;
    }

    // Free all blocks
    for (int i = 0; i < DIRECT_BLOCKS && i < inode->block_count; i++) {
        if (inode->direct_blocks[i] > 0) {
            block_free(fs, inode->direct_blocks[i]);
        }
    }

    // Remove from B-Tree
    btree_delete(&fs->inode_tree, inode->inode_num);

    // Clear bitmap
    bitmap_clear(fs->inode_bitmap, inode->inode_num);

    inode->in_use = 0;
    fs->free_inodes++;
}

// Get i-node by number
struct inode *inode_get(struct btree_filesystem *fs, uint32_t inode_num) {
    return (struct inode *)btree_search(&fs->inode_tree, inode_num);
}

// Allocate a block
uint32_t block_alloc(struct btree_filesystem *fs) {
    // Start from block 1 since block 0 is reserved (indicates no block)
    for (int i = 1; i < MAX_BLOCKS; i++) {
        if (!bitmap_test(fs->block_bitmap, i)) {
            bitmap_set(fs->block_bitmap, i);
            fs->free_blocks--;

            // Clear block
            uint8_t *block_ptr = block_get_ptr(fs, i);
            if (block_ptr) {
                memset(block_ptr, 0, FS_BLOCK_SIZE);
            }

            return i;
        }
    }

    // No free blocks
    return 0;
}

// Free a block
void block_free(struct btree_filesystem *fs, uint32_t block_num) {
    if (block_num == 0 || block_num >= MAX_BLOCKS) {
        return;
    }

    bitmap_clear(fs->block_bitmap, block_num);
    fs->free_blocks++;
}

// Get pointer to block
uint8_t *block_get_ptr(struct btree_filesystem *fs, uint32_t block_num) {
    if (block_num == 0 || block_num >= MAX_BLOCKS) {
        return NULL;
    }
    return fs->block_storage + (block_num * FS_BLOCK_SIZE);
}

// Check if block is allocated
int block_is_allocated(struct btree_filesystem *fs, uint32_t block_num) {
    return bitmap_test(fs->block_bitmap, block_num);
}

// Read data from i-node
int inode_read(struct btree_filesystem *fs, struct inode *inode, void *buffer, uint32_t offset, uint32_t size) {
    if (!inode || offset >= inode->size) {
        return 0;
    }

    if (offset + size > inode->size) {
        size = inode->size - offset;
    }

    uint32_t bytes_read = 0;
    uint8_t *buf = (uint8_t *)buffer;

    while (bytes_read < size) {
        uint32_t block_idx = (offset + bytes_read) / FS_BLOCK_SIZE;
        uint32_t block_offset = (offset + bytes_read) % FS_BLOCK_SIZE;
        uint32_t bytes_to_read = FS_BLOCK_SIZE - block_offset;

        if (bytes_to_read > size - bytes_read) {
            bytes_to_read = size - bytes_read;
        }

        // Only support direct blocks for now
        if (block_idx >= DIRECT_BLOCKS) {
            break;
        }

        uint32_t block_num = inode->direct_blocks[block_idx];
        if (block_num == 0) {
            break;
        }

        uint8_t *block_ptr = block_get_ptr(fs, block_num);
        if (!block_ptr) {
            break;
        }

        // Copy data
        for (uint32_t i = 0; i < bytes_to_read; i++) {
            buf[bytes_read + i] = block_ptr[block_offset + i];
        }

        bytes_read += bytes_to_read;
    }

    return bytes_read;
}

// Write data to i-node
int inode_write(struct btree_filesystem *fs, struct inode *inode, const void *data, uint32_t offset, uint32_t size) {
    if (!inode || !data) {
        return 0;
    }

    uint32_t bytes_written = 0;
    const uint8_t *buf = (const uint8_t *)data;

    while (bytes_written < size) {
        uint32_t block_idx = (offset + bytes_written) / FS_BLOCK_SIZE;
        uint32_t block_offset = (offset + bytes_written) % FS_BLOCK_SIZE;
        uint32_t bytes_to_write = FS_BLOCK_SIZE - block_offset;

        if (bytes_to_write > size - bytes_written) {
            bytes_to_write = size - bytes_written;
        }

        // Only support direct blocks for now
        if (block_idx >= DIRECT_BLOCKS) {
            break;
        }

        // Allocate block if needed
        if (inode->direct_blocks[block_idx] == 0) {
            uint32_t new_block = block_alloc(fs);
            if (new_block == 0) {
                break;
            }
            inode->direct_blocks[block_idx] = new_block;
            inode->block_count++;
        }

        uint32_t block_num = inode->direct_blocks[block_idx];
        uint8_t *block_ptr = block_get_ptr(fs, block_num);
        if (!block_ptr) {
            break;
        }

        // Copy data
        for (uint32_t i = 0; i < bytes_to_write; i++) {
            block_ptr[block_offset + i] = buf[bytes_written + i];
        }

        bytes_written += bytes_to_write;
    }

    // Update i-node size
    if (offset + bytes_written > inode->size) {
        inode->size = offset + bytes_written;
    }

    return bytes_written;
}

// Truncate i-node to new size
int inode_truncate(struct btree_filesystem *fs, struct inode *inode, uint32_t new_size) {
    if (!inode) {
        return -1;
    }

    if (new_size >= inode->size) {
        return 0;
    }

    uint32_t new_blocks = (new_size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;

    // Free blocks beyond new size
    for (uint32_t i = new_blocks; i < inode->block_count && i < DIRECT_BLOCKS; i++) {
        if (inode->direct_blocks[i] > 0) {
            block_free(fs, inode->direct_blocks[i]);
            inode->direct_blocks[i] = 0;
        }
    }

    inode->size = new_size;
    inode->block_count = new_blocks;

    return 0;
}

// Create a file
int btree_fs_create(struct btree_filesystem *fs, const char *filename, uint32_t type) {
    if (strlen(filename) >= MAX_FILENAME_LEN) {
        printf("Error: Filename too long\n");
        return -1;
    }

    // Check if file already exists
    uint32_t hash = hash_string(filename);
    struct inode *existing = (struct inode *)btree_search(&fs->name_tree, hash);
    if (existing) {
        printf("Error: File already exists\n");
        return -1;
    }

    // Allocate i-node
    struct inode *inode = inode_alloc(fs, type);
    if (!inode) {
        return -1;
    }

    // Add filename mapping to name tree
    btree_insert(&fs->name_tree, hash, inode);

    printf("Created file '%s' with i-node %u\n", filename, inode->inode_num);
    return inode->inode_num;
}

// Open a file (get i-node number)
int btree_fs_open(struct btree_filesystem *fs, const char *filename) {
    uint32_t hash = hash_string(filename);
    struct inode *inode = (struct inode *)btree_search(&fs->name_tree, hash);

    if (!inode) {
        return -1;
    }

    return inode->inode_num;
}

// Read from file
int btree_fs_read(struct btree_filesystem *fs, const char *filename, void *buffer, uint32_t size) {
    int inode_num = btree_fs_open(fs, filename);
    if (inode_num < 0) {
        printf("Error: File not found\n");
        return -1;
    }

    struct inode *inode = inode_get(fs, inode_num);
    if (!inode) {
        return -1;
    }

    return inode_read(fs, inode, buffer, 0, size);
}

// Write to file
int btree_fs_write(struct btree_filesystem *fs, const char *filename, const void *data, uint32_t size) {
    int inode_num = btree_fs_open(fs, filename);
    if (inode_num < 0) {
        printf("Error: File not found\n");
        return -1;
    }

    struct inode *inode = inode_get(fs, inode_num);
    if (!inode) {
        return -1;
    }

    return inode_write(fs, inode, data, 0, size);
}

// Delete a file
int btree_fs_delete(struct btree_filesystem *fs, const char *filename) {
    uint32_t hash = hash_string(filename);
    struct inode *inode = (struct inode *)btree_search(&fs->name_tree, hash);

    if (!inode) {
        printf("Error: File not found\n");
        return -1;
    }

    // Remove from name tree
    btree_delete(&fs->name_tree, hash);

    // Free i-node
    inode_free(fs, inode);

    printf("Deleted file '%s'\n", filename);
    return 0;
}

// List files callback
static void list_callback(uint32_t key, void *value) {
    struct inode *inode = (struct inode *)value;
    if (inode && inode->in_use) {
        printf("  i-node %u: size=%u bytes, blocks=%u, type=%u\n",
               inode->inode_num, inode->size, inode->block_count, inode->type);
    }
}

// List all files
void btree_fs_list(struct btree_filesystem *fs) {
    printf("Files in B-Tree filesystem:\n");
    btree_traverse(&fs->name_tree, list_callback);
}

// Print file stats
void btree_fs_stat(struct btree_filesystem *fs, const char *filename) {
    int inode_num = btree_fs_open(fs, filename);
    if (inode_num < 0) {
        printf("Error: File not found\n");
        return;
    }

    struct inode *inode = inode_get(fs, inode_num);
    if (!inode) {
        return;
    }

    inode_print(inode);
}

// Print i-node information
void inode_print(struct inode *inode) {
    if (!inode) {
        return;
    }

    printf("I-node %u:\n", inode->inode_num);
    printf("  Type: %u\n", inode->type);
    printf("  Size: %u bytes\n", inode->size);
    printf("  Blocks: %u\n", inode->block_count);
    printf("  Permissions: 0x%x\n", inode->permissions);
    printf("  Links: %u\n", inode->link_count);
    printf("  Direct blocks: ");
    for (int i = 0; i < DIRECT_BLOCKS && i < inode->block_count; i++) {
        printf("%u ", inode->direct_blocks[i]);
    }
    printf("\n");
}

// Print filesystem statistics
void fs_print_stats(struct btree_filesystem *fs) {
    printf("B-Tree Filesystem Statistics:\n");
    printf("  Total i-nodes: %d\n", fs->total_inodes);
    printf("  Free i-nodes: %d\n", fs->free_inodes);
    printf("  Used i-nodes: %d\n", fs->total_inodes - fs->free_inodes);
    printf("  Total blocks: %d\n", fs->total_blocks);
    printf("  Free blocks: %d\n", fs->free_blocks);
    printf("  Used blocks: %d\n", fs->total_blocks - fs->free_blocks);
    printf("  Block size: %d bytes\n", FS_BLOCK_SIZE);
    printf("  Total storage: %d KB\n", (fs->total_blocks * FS_BLOCK_SIZE) / 1024);
    printf("  Used storage: %d KB\n", ((fs->total_blocks - fs->free_blocks) * FS_BLOCK_SIZE) / 1024);
}
