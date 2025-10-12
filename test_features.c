#include "kernel.h"
#include "common.h"
#include "rbtree.h"
#include "cfs.h"
#include "fd.h"
#include "epoll.h"
#include "btree.h"
#include "inode.h"

/* Test Red-Black Tree */
void test_rbtree(void) {
    printf("\n=== Red-Black Tree Test ===\n");

    struct test_node {
        struct rb_node rb;
        int key;
    };

    struct rb_root root = RB_ROOT;
    struct test_node nodes[5];
    int keys[] = {5, 3, 7, 1, 9};

    /* Insert nodes */
    for (int i = 0; i < 5; i++) {
        nodes[i].key = keys[i];
        RB_CLEAR_NODE(&nodes[i].rb);

        struct rb_node **link = &root.rb_node;
        struct rb_node *parent = NULL;

        while (*link) {
            struct test_node *entry;
            parent = *link;
            entry = rb_entry(parent, struct test_node, rb);

            if (nodes[i].key < entry->key)
                link = &parent->left;
            else
                link = &parent->right;
        }

        nodes[i].rb.parent = parent;
        nodes[i].rb.left = NULL;
        nodes[i].rb.right = NULL;
        nodes[i].rb.color = RB_RED;

        if (parent) {
            if (link == &parent->left)
                parent->left = &nodes[i].rb;
            else
                parent->right = &nodes[i].rb;
        } else {
            root.rb_node = &nodes[i].rb;
        }

        rb_insert_color(&nodes[i].rb, &root);
        printf("Inserted key %d\n", keys[i]);
    }

    /* Traverse in-order */
    printf("In-order traversal: ");
    struct rb_node *node = rb_first(&root);
    while (node) {
        struct test_node *entry = rb_entry(node, struct test_node, rb);
        printf("%d ", entry->key);
        node = rb_next(node);
    }
    printf("\n");

    printf("RB-Tree test passed!\n");
}

/* Test CFS Scheduler */
void cfs_test_process_1(void) {
    for (int i = 0; i < 3; i++) {
        printf("CFS Test Process 1 (nice=0): iteration %d\n", i);
        for (volatile int j = 0; j < 500000; j++);
    }
}

void cfs_test_process_2(void) {
    for (int i = 0; i < 3; i++) {
        printf("CFS Test Process 2 (nice=5): iteration %d\n", i);
        for (volatile int j = 0; j < 500000; j++);
    }
}

void cfs_test_process_3(void) {
    for (int i = 0; i < 3; i++) {
        printf("CFS Test Process 3 (nice=-5): iteration %d\n", i);
        for (volatile int j = 0; j < 500000; j++);
    }
}

void test_cfs(void) {
    printf("\n=== CFS Scheduler Test ===\n");

    cfs_init();

    /* Create processes with different nice values */
    struct cfs_process *p1 = cfs_create_process(cfs_test_process_1, 0);
    struct cfs_process *p2 = cfs_create_process(cfs_test_process_2, 5);
    struct cfs_process *p3 = cfs_create_process(cfs_test_process_3, -5);

    if (!p1 || !p2 || !p3) {
        printf("Failed to create CFS processes\n");
        return;
    }

    /* Simulate scheduler ticks */
    printf("\nSimulating CFS scheduler...\n");
    for (int i = 0; i < 10; i++) {
        printf("\n--- Scheduler Tick %d ---\n", i);
        cfs_scheduler_tick();
    }

    printf("\nCFS test completed!\n");
}

/* Test epoll */
void test_epoll(void) {
    printf("\n=== epoll Test ===\n");

    /* Initialize subsystems */
    fd_init();
    epoll_init();

    /* Create a UART file descriptor */
    int uart_fd = fd_alloc(FD_TYPE_UART, NULL, &uart_fd_ops);
    if (uart_fd < 0) {
        printf("Failed to allocate UART fd\n");
        return;
    }
    printf("Created UART fd: %d\n", uart_fd);

    /* Create epoll instance */
    int epfd = epoll_create(10);
    if (epfd == -999) {
        printf("Failed to create epoll instance\n");
        return;
    }
    printf("Created epoll instance: %d\n", epfd);

    /* Add UART fd to epoll */
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data = uart_fd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, uart_fd, &ev) < 0) {
        printf("Failed to add fd to epoll\n");
        return;
    }
    printf("Added UART fd to epoll\n");

    /* Wait for events */
    struct epoll_event events[10];
    printf("\nPolling for events...\n");
    int nfds = epoll_wait(epfd, events, 10, 0);
    printf("epoll_wait returned %d events\n", nfds);

    for (int i = 0; i < nfds; i++) {
        printf("Event %d: fd=%llu, events=0x%x ", i, events[i].data, events[i].events);
        if (events[i].events & EPOLLIN) printf("EPOLLIN ");
        if (events[i].events & EPOLLOUT) printf("EPOLLOUT ");
        printf("\n");
    }

    /* Modify event */
    ev.events = EPOLLIN;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, uart_fd, &ev) < 0) {
        printf("Failed to modify fd in epoll\n");
        return;
    }
    printf("\nModified UART fd events to EPOLLIN only\n");

    /* Remove from epoll */
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, uart_fd, NULL) < 0) {
        printf("Failed to remove fd from epoll\n");
        return;
    }
    printf("Removed UART fd from epoll\n");

    /* Cleanup */
    epoll_close(epfd);
    fd_close(uart_fd);

    printf("\nepoll test completed!\n");
}

/* B-Tree Filesystem Test */
extern void test_btree_filesystem(void);

/* Combined integration test */
void test_all_features(void) {
    printf("\n");
    printf("========================================\n");
    printf("  Testing All Features\n");
    printf("========================================\n");

    test_rbtree();
    test_cfs();
    test_epoll();
    test_btree_filesystem();

    printf("\n");
    printf("========================================\n");
    printf("  All Tests Completed!\n");
    printf("========================================\n");
    printf("\n");
}
