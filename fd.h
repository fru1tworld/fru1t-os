#pragma once
#include "kernel.h"

/* File descriptor abstraction for epoll */

#define MAX_FDS 64
#define FD_TYPE_UNUSED 0
#define FD_TYPE_FILE   1
#define FD_TYPE_UART   2
#define FD_TYPE_PIPE   3
#define FD_TYPE_SOCKET 4

/* File descriptor flags */
#define FD_READABLE  (1 << 0)
#define FD_WRITABLE  (1 << 1)
#define FD_ERROR     (1 << 2)
#define FD_HANGUP    (1 << 3)

/* File descriptor operations */
struct fd_ops {
    int (*read)(void *ctx, void *buf, size_t count);
    int (*write)(void *ctx, const void *buf, size_t count);
    int (*poll)(void *ctx);  /* Returns FD_READABLE/FD_WRITABLE/etc flags */
    void (*close)(void *ctx);
};

/* File descriptor structure */
struct fd {
    int fd_num;              /* File descriptor number */
    int type;                /* FD_TYPE_* */
    int flags;               /* Current status flags */
    void *context;           /* Type-specific context */
    struct fd_ops *ops;      /* Operations */
    int ref_count;           /* Reference count */
};

/* File descriptor table (per process, but we'll use global for simplicity) */
struct fd_table {
    struct fd fds[MAX_FDS];
    int next_fd;
};

/* Initialize file descriptor subsystem */
void fd_init(void);

/* Allocate a new file descriptor */
int fd_alloc(int type, void *context, struct fd_ops *ops);

/* Get file descriptor structure */
struct fd *fd_get(int fd_num);

/* Close a file descriptor */
int fd_close(int fd_num);

/* Poll file descriptor for events */
int fd_poll(int fd_num);

/* Update file descriptor flags */
void fd_update_flags(int fd_num, int flags);

/* UART-specific file descriptor operations */
extern struct fd_ops uart_fd_ops;

/* Global file descriptor table */
extern struct fd_table global_fd_table;
