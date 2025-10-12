#include "fd.h"
#include "common.h"

/* Global file descriptor table */
struct fd_table global_fd_table;

/* UART file descriptor operations */
static int uart_fd_read(void *ctx, void *buf, size_t count) {
    (void)ctx;
    char *cbuf = (char *)buf;
    size_t i;

    for (i = 0; i < count; i++) {
        if (!uart_rx_ready()) {
            break;
        }
        cbuf[i] = uart_getchar();
    }

    return (int)i;
}

static int uart_fd_write(void *ctx, const void *buf, size_t count) {
    (void)ctx;
    const char *cbuf = (const char *)buf;

    for (size_t i = 0; i < count; i++) {
        uart_putchar(cbuf[i]);
    }

    return (int)count;
}

static int uart_fd_poll(void *ctx) {
    (void)ctx;
    int flags = 0;

    /* UART is always writable */
    flags |= FD_WRITABLE;

    /* Check if readable */
    if (uart_rx_ready()) {
        flags |= FD_READABLE;
    }

    return flags;
}

static void uart_fd_close(void *ctx) {
    (void)ctx;
    /* UART can't really be closed */
}

struct fd_ops uart_fd_ops = {
    .read = uart_fd_read,
    .write = uart_fd_write,
    .poll = uart_fd_poll,
    .close = uart_fd_close,
};

/* Initialize file descriptor subsystem */
void fd_init(void) {
    for (int i = 0; i < MAX_FDS; i++) {
        global_fd_table.fds[i].fd_num = i;
        global_fd_table.fds[i].type = FD_TYPE_UNUSED;
        global_fd_table.fds[i].flags = 0;
        global_fd_table.fds[i].context = NULL;
        global_fd_table.fds[i].ops = NULL;
        global_fd_table.fds[i].ref_count = 0;
    }
    global_fd_table.next_fd = 0;

    printf("File descriptor subsystem initialized\n");
}

/* Allocate a new file descriptor */
int fd_alloc(int type, void *context, struct fd_ops *ops) {
    for (int i = 0; i < MAX_FDS; i++) {
        int fd_num = (global_fd_table.next_fd + i) % MAX_FDS;
        struct fd *fd = &global_fd_table.fds[fd_num];

        if (fd->type == FD_TYPE_UNUSED) {
            fd->type = type;
            fd->flags = 0;
            fd->context = context;
            fd->ops = ops;
            fd->ref_count = 1;

            global_fd_table.next_fd = (fd_num + 1) % MAX_FDS;

            printf("FD: Allocated fd %d (type=%d)\n", fd_num, type);
            return fd_num;
        }
    }

    printf("FD: No free file descriptors\n");
    return -1;
}

/* Get file descriptor structure */
struct fd *fd_get(int fd_num) {
    if (fd_num < 0 || fd_num >= MAX_FDS) {
        return NULL;
    }

    struct fd *fd = &global_fd_table.fds[fd_num];
    if (fd->type == FD_TYPE_UNUSED) {
        return NULL;
    }

    return fd;
}

/* Close a file descriptor */
int fd_close(int fd_num) {
    struct fd *fd = fd_get(fd_num);
    if (!fd) {
        return -1;
    }

    fd->ref_count--;
    if (fd->ref_count <= 0) {
        if (fd->ops && fd->ops->close) {
            fd->ops->close(fd->context);
        }

        fd->type = FD_TYPE_UNUSED;
        fd->flags = 0;
        fd->context = NULL;
        fd->ops = NULL;
        fd->ref_count = 0;

        printf("FD: Closed fd %d\n", fd_num);
    }

    return 0;
}

/* Poll file descriptor for events */
int fd_poll(int fd_num) {
    struct fd *fd = fd_get(fd_num);
    if (!fd || !fd->ops || !fd->ops->poll) {
        return 0;
    }

    int flags = fd->ops->poll(fd->context);
    fd->flags = flags;
    return flags;
}

/* Update file descriptor flags */
void fd_update_flags(int fd_num, int flags) {
    struct fd *fd = fd_get(fd_num);
    if (fd) {
        fd->flags = flags;
    }
}
