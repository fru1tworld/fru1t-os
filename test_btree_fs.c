#include "kernel.h"
#include "btree.h"
#include "inode.h"

// String length helper
static int strlen(const char *s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

// 전역 파일시스템 인스턴스
static struct btree_filesystem g_fs;

void test_btree_basic(void) {
    printf("\n=== Testing B-Tree Basic Operations ===\n");

    struct btree tree;
    btree_init(&tree);

    // Test insertions
    printf("Inserting keys: 10, 20, 5, 6, 12, 30, 7, 17\n");
    btree_insert(&tree, 10, (void *)100);
    btree_insert(&tree, 20, (void *)200);
    btree_insert(&tree, 5, (void *)50);
    btree_insert(&tree, 6, (void *)60);
    btree_insert(&tree, 12, (void *)120);
    btree_insert(&tree, 30, (void *)300);
    btree_insert(&tree, 7, (void *)70);
    btree_insert(&tree, 17, (void *)170);

    btree_print(&tree);

    // Test search
    printf("\nSearching for key 6: ");
    void *val = btree_search(&tree, 6);
    printf("Found value: %u\n", (uint32_t)val);

    printf("Searching for key 99: ");
    val = btree_search(&tree, 99);
    printf("%s\n", val ? "Found" : "Not found");

    btree_destroy(&tree);
    printf("B-Tree test completed\n");

    // Initialize global filesystem for subsequent tests
    printf("\nInitializing global B-Tree filesystem...\n");
    inode_fs_init(&g_fs);
}

void test_inode_operations(void) {
    printf("\n=== Testing I-node Operations ===\n");

    // Filesystem already initialized by test_btree_basic
    // inode_fs_init(&g_fs);

    // Test i-node allocation
    printf("\nAllocating i-nodes:\n");
    struct inode *inode1 = inode_alloc(&g_fs, INODE_TYPE_FILE);
    struct inode *inode2 = inode_alloc(&g_fs, INODE_TYPE_FILE);
    struct inode *inode3 = inode_alloc(&g_fs, INODE_TYPE_DIR);

    printf("Allocated i-node %u (file)\n", inode1->inode_num);
    printf("Allocated i-node %u (file)\n", inode2->inode_num);
    printf("Allocated i-node %u (directory)\n", inode3->inode_num);

    // Test block allocation
    printf("\nAllocating blocks:\n");
    uint32_t block1 = block_alloc(&g_fs);
    uint32_t block2 = block_alloc(&g_fs);
    printf("Allocated blocks: %u, %u\n", block1, block2);

    // Test i-node write/read
    printf("\nTesting i-node write/read:\n");
    const char *test_data = "Hello, B-Tree Filesystem!";
    int written = inode_write(&g_fs, inode1, test_data, 0, strlen(test_data) + 1);
    printf("Written %d bytes to i-node %u\n", written, inode1->inode_num);

    char read_buffer[128];
    memset(read_buffer, 0, sizeof(read_buffer));
    int read = inode_read(&g_fs, inode1, read_buffer, 0, sizeof(read_buffer));
    printf("Read %d bytes: '%s'\n", read, read_buffer);

    // Print i-node info
    inode_print(inode1);

    printf("I-node test completed\n");
}

void test_file_operations(void) {
    printf("\n=== Testing File Operations ===\n");

    // Create files
    printf("\nCreating files:\n");
    btree_fs_create(&g_fs, "test.txt", INODE_TYPE_FILE);
    btree_fs_create(&g_fs, "data.bin", INODE_TYPE_FILE);
    btree_fs_create(&g_fs, "readme.md", INODE_TYPE_FILE);

    // Write to files
    printf("\nWriting to files:\n");
    const char *content1 = "This is a test file in B-Tree filesystem.";
    const char *content2 = "Binary data: 0x12345678";

    btree_fs_write(&g_fs, "test.txt", content1, strlen(content1) + 1);
    btree_fs_write(&g_fs, "data.bin", content2, strlen(content2) + 1);

    // Read from files
    printf("\nReading from files:\n");
    char buffer[256];

    memset(buffer, 0, sizeof(buffer));
    int bytes_read = btree_fs_read(&g_fs, "test.txt", buffer, sizeof(buffer));
    printf("Read %d bytes from test.txt: '%s'\n", bytes_read, buffer);

    memset(buffer, 0, sizeof(buffer));
    bytes_read = btree_fs_read(&g_fs, "data.bin", buffer, sizeof(buffer));
    printf("Read %d bytes from data.bin: '%s'\n", bytes_read, buffer);

    // List files
    printf("\n");
    btree_fs_list(&g_fs);

    // Print file stats
    printf("\n");
    btree_fs_stat(&g_fs, "test.txt");

    // Delete a file
    printf("\n");
    btree_fs_delete(&g_fs, "readme.md");

    // List files again
    printf("\n");
    btree_fs_list(&g_fs);

    // Print filesystem stats
    printf("\n");
    fs_print_stats(&g_fs);

    printf("\nFile operations test completed\n");
}

void test_large_file(void) {
    printf("\n=== Testing Large File ===\n");

    btree_fs_create(&g_fs, "large.dat", INODE_TYPE_FILE);

    // Write multiple blocks
    char data_block[512];
    for (int i = 0; i < 512; i++) {
        data_block[i] = 'A' + (i % 26);
    }

    int inode_num = btree_fs_open(&g_fs, "large.dat");
    struct inode *inode = inode_get(&g_fs, inode_num);

    printf("Writing multiple blocks...\n");
    for (int i = 0; i < 3; i++) {
        int written = inode_write(&g_fs, inode, data_block, i * 512, 512);
        printf("Block %d: written %d bytes\n", i, written);
    }

    printf("File size: %u bytes (%u blocks)\n", inode->size, inode->block_count);

    // Read back
    char read_buf[512];
    memset(read_buf, 0, sizeof(read_buf));
    int read = inode_read(&g_fs, inode, read_buf, 512, 512);
    printf("Read %d bytes from second block (first 50 chars): %.50s\n", read, read_buf);

    fs_print_stats(&g_fs);

    printf("Large file test completed\n");
}

void test_btree_filesystem(void) {
    printf("\n========================================\n");
    printf("  B-Tree Filesystem Test Suite\n");
    printf("========================================\n");

    test_btree_basic();
    test_inode_operations();
    test_file_operations();
    test_large_file();

    printf("\n========================================\n");
    printf("  All tests completed successfully!\n");
    printf("========================================\n");
}
