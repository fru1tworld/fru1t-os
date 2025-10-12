#include "epoll.h"
#include "common.h"

/* Global epoll instances */
struct epoll_instances global_epoll;

/* Initialize epoll subsystem */
void epoll_init(void) {
    for (int i = 0; i < MAX_EPOLL_INSTANCES; i++) {
        global_epoll.instances[i].epfd = -1;
        global_epoll.instances[i].items_tree = RB_ROOT;
        global_epoll.instances[i].ready_list = NULL;
        global_epoll.instances[i].num_items = 0;
        global_epoll.instances[i].in_use = 0;
    }

    printf("epoll subsystem initialized\n");
}

/* Get epoll instance */
struct epoll_instance *epoll_get_instance(int epfd) {
    for (int i = 0; i < MAX_EPOLL_INSTANCES; i++) {
        if (global_epoll.instances[i].in_use &&
            global_epoll.instances[i].epfd == epfd) {
            return &global_epoll.instances[i];
        }
    }
    return NULL;
}

/* Create an epoll instance */
int epoll_create(int size) {
    (void)size;  /* Size hint is ignored in modern Linux too */

    for (int i = 0; i < MAX_EPOLL_INSTANCES; i++) {
        if (!global_epoll.instances[i].in_use) {
            struct epoll_instance *epi = &global_epoll.instances[i];

            /* Allocate an epoll fd (using negative numbers to distinguish) */
            epi->epfd = -(i + 1);
            epi->items_tree = RB_ROOT;
            epi->ready_list = NULL;
            epi->num_items = 0;
            epi->in_use = 1;

            printf("epoll: Created epoll instance %d\n", epi->epfd);
            return epi->epfd;
        }
    }

    printf("epoll: No free epoll instances\n");
    return -999;  /* Error indicator different from valid epfd */
}

/* Find item in RB-tree by fd */
struct epoll_item *epoll_find_item(struct epoll_instance *epi, int fd) {
    struct rb_node *node = epi->items_tree.rb_node;

    while (node) {
        struct epoll_item *item = rb_entry(node, struct epoll_item, rb_node);

        if (fd < item->fd) {
            node = node->left;
        } else if (fd > item->fd) {
            node = node->right;
        } else {
            return item;
        }
    }

    return NULL;
}

/* Insert item into RB-tree */
static int epoll_insert_item(struct epoll_instance *epi, struct epoll_item *new_item) {
    struct rb_node **link = &epi->items_tree.rb_node;
    struct rb_node *parent = NULL;

    /* Find insertion point */
    while (*link) {
        struct epoll_item *item;
        parent = *link;
        item = rb_entry(parent, struct epoll_item, rb_node);

        if (new_item->fd < item->fd) {
            link = &parent->left;
        } else if (new_item->fd > item->fd) {
            link = &parent->right;
        } else {
            /* Duplicate fd */
            return -1;
        }
    }

    /* Insert into RB-tree */
    new_item->rb_node.parent = parent;
    new_item->rb_node.left = NULL;
    new_item->rb_node.right = NULL;
    new_item->rb_node.color = RB_RED;

    if (parent) {
        if (link == &parent->left)
            parent->left = &new_item->rb_node;
        else
            parent->right = &new_item->rb_node;
    } else {
        epi->items_tree.rb_node = &new_item->rb_node;
    }

    rb_insert_color(&new_item->rb_node, &epi->items_tree);
    epi->num_items++;

    return 0;
}

/* Control interface for epoll */
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
    struct epoll_instance *epi = epoll_get_instance(epfd);
    if (!epi) {
        printf("epoll_ctl: Invalid epfd %d\n", epfd);
        return -1;
    }

    /* Verify fd exists */
    struct fd *fd_entry = fd_get(fd);
    if (!fd_entry) {
        printf("epoll_ctl: Invalid fd %d\n", fd);
        return -1;
    }

    switch (op) {
        case EPOLL_CTL_ADD: {
            /* Check if fd already exists */
            if (epoll_find_item(epi, fd)) {
                printf("epoll_ctl: fd %d already in epoll instance\n", fd);
                return -1;
            }

            /* Allocate new item */
            struct epoll_item *item = (struct epoll_item *)kmalloc(sizeof(struct epoll_item));
            if (!item) {
                printf("epoll_ctl: Failed to allocate epoll_item\n");
                return -1;
            }

            item->fd = fd;
            item->events = event->events;
            item->user_data = event->data;
            item->revents = 0;
            RB_CLEAR_NODE(&item->rb_node);

            if (epoll_insert_item(epi, item) < 0) {
                kfree(item);
                return -1;
            }

            printf("epoll_ctl: Added fd %d to epoll %d (events=0x%x)\n",
                   fd, epfd, event->events);
            break;
        }

        case EPOLL_CTL_DEL: {
            struct epoll_item *item = epoll_find_item(epi, fd);
            if (!item) {
                printf("epoll_ctl: fd %d not found in epoll instance\n", fd);
                return -1;
            }

            rb_erase(&item->rb_node, &epi->items_tree);
            epi->num_items--;
            kfree(item);

            printf("epoll_ctl: Removed fd %d from epoll %d\n", fd, epfd);
            break;
        }

        case EPOLL_CTL_MOD: {
            struct epoll_item *item = epoll_find_item(epi, fd);
            if (!item) {
                printf("epoll_ctl: fd %d not found in epoll instance\n", fd);
                return -1;
            }

            item->events = event->events;
            item->user_data = event->data;

            printf("epoll_ctl: Modified fd %d in epoll %d (events=0x%x)\n",
                   fd, epfd, event->events);
            break;
        }

        default:
            printf("epoll_ctl: Invalid operation %d\n", op);
            return -1;
    }

    return 0;
}

/* Poll all monitored fds */
void epoll_poll_fds(struct epoll_instance *epi) {
    struct rb_node *node = rb_first(&epi->items_tree);

    while (node) {
        struct epoll_item *item = rb_entry(node, struct epoll_item, rb_node);
        int fd_flags = fd_poll(item->fd);
        uint32_t revents = 0;

        /* Convert fd flags to epoll events */
        if (fd_flags & FD_READABLE) {
            revents |= EPOLLIN;
        }
        if (fd_flags & FD_WRITABLE) {
            revents |= EPOLLOUT;
        }
        if (fd_flags & FD_ERROR) {
            revents |= EPOLLERR;
        }
        if (fd_flags & FD_HANGUP) {
            revents |= EPOLLHUP;
        }

        /* Check if any requested events occurred */
        item->revents = revents & item->events;

        node = rb_next(node);
    }
}

/* Wait for events */
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
    struct epoll_instance *epi = epoll_get_instance(epfd);
    if (!epi) {
        printf("epoll_wait: Invalid epfd %d\n", epfd);
        return -1;
    }

    if (maxevents <= 0) {
        printf("epoll_wait: Invalid maxevents %d\n", maxevents);
        return -1;
    }

    /* Poll all fds to update their status */
    epoll_poll_fds(epi);

    /* Collect ready events */
    int num_ready = 0;
    struct rb_node *node = rb_first(&epi->items_tree);

    while (node && num_ready < maxevents) {
        struct epoll_item *item = rb_entry(node, struct epoll_item, rb_node);

        if (item->revents) {
            events[num_ready].events = item->revents;
            events[num_ready].data = item->user_data;
            num_ready++;

            printf("epoll_wait: fd %d ready (events=0x%x)\n",
                   item->fd, item->revents);
        }

        node = rb_next(node);
    }

    if (num_ready == 0 && timeout != 0) {
        /* In a real implementation, we would sleep here */
        printf("epoll_wait: No events ready (would block with timeout=%d)\n", timeout);
    }

    return num_ready;
}

/* Close epoll instance */
int epoll_close(int epfd) {
    struct epoll_instance *epi = epoll_get_instance(epfd);
    if (!epi) {
        printf("epoll_close: Invalid epfd %d\n", epfd);
        return -1;
    }

    /* Free all items */
    struct rb_node *node = rb_first(&epi->items_tree);
    while (node) {
        struct epoll_item *item = rb_entry(node, struct epoll_item, rb_node);
        struct rb_node *next = rb_next(node);

        rb_erase(&item->rb_node, &epi->items_tree);
        kfree(item);

        node = next;
    }

    /* Clear instance */
    epi->epfd = -1;
    epi->items_tree = RB_ROOT;
    epi->ready_list = NULL;
    epi->num_items = 0;
    epi->in_use = 0;

    printf("epoll: Closed epoll instance %d\n", epfd);
    return 0;
}
