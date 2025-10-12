#pragma once
#include "kernel.h"
#include "rbtree.h"
#include "fd.h"

/* epoll implementation using Red-Black Tree */

#define MAX_EPOLL_INSTANCES 16
#define MAX_EVENTS_PER_EPOLL 128

/* epoll event flags (compatible with Linux epoll) */
#define EPOLLIN      0x001  /* Readable */
#define EPOLLOUT     0x004  /* Writable */
#define EPOLLERR     0x008  /* Error condition */
#define EPOLLHUP     0x010  /* Hang up */
#define EPOLLET      0x80000000  /* Edge-triggered mode */

/* epoll_ctl operations */
#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3

/* epoll_event structure */
struct epoll_event {
    uint32_t events;   /* Epoll events */
    uint64_t data;     /* User data */
};

/* Internal epoll item stored in RB-tree */
struct epoll_item {
    struct rb_node rb_node;  /* RB-tree linkage */
    int fd;                  /* File descriptor being monitored */
    uint32_t events;         /* Events of interest */
    uint64_t user_data;      /* User data */
    uint32_t revents;        /* Returned events */
};

/* epoll instance */
struct epoll_instance {
    int epfd;                           /* epoll file descriptor */
    struct rb_root items_tree;          /* RB-tree of monitored fds */
    struct epoll_item *ready_list;      /* List of ready items */
    int num_items;                      /* Number of monitored items */
    int in_use;                         /* Is this instance in use? */
};

/* Global epoll instances */
struct epoll_instances {
    struct epoll_instance instances[MAX_EPOLL_INSTANCES];
};

/* Initialize epoll subsystem */
void epoll_init(void);

/* Create an epoll instance */
int epoll_create(int size);

/* Control interface for an epoll file descriptor */
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

/* Wait for events on an epoll file descriptor */
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

/* Close an epoll instance */
int epoll_close(int epfd);

/* Helper: Get epoll instance */
struct epoll_instance *epoll_get_instance(int epfd);

/* Helper: Find item in RB-tree */
struct epoll_item *epoll_find_item(struct epoll_instance *epi, int fd);

/* Helper: Poll all monitored fds and update ready list */
void epoll_poll_fds(struct epoll_instance *epi);

/* Global epoll instances */
extern struct epoll_instances global_epoll;
