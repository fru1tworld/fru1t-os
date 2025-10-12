#pragma once
#include "kernel.h"
#include "rbtree.h"
#include "fd.h"

/* 레드-블랙 트리를 사용한 epoll 구현 */

#define MAX_EPOLL_INSTANCES 16
#define MAX_EVENTS_PER_EPOLL 128

/* epoll 이벤트 플래그 (Linux epoll과 호환) */
#define EPOLLIN      0x001  /* 읽기 가능 */
#define EPOLLOUT     0x004  /* 쓰기 가능 */
#define EPOLLERR     0x008  /* 에러 조건 */
#define EPOLLHUP     0x010  /* 연결 끊김 */
#define EPOLLET      0x80000000  /* 엣지 트리거 모드 */

/* epoll_ctl 연산 */
#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3

/* epoll_event 구조체 */
struct epoll_event {
    uint32_t events;   /* Epoll 이벤트 */
    uint64_t data;     /* 사용자 데이터 */
};

/* RB 트리에 저장되는 내부 epoll 아이템 */
struct epoll_item {
    struct rb_node rb_node;  /* RB 트리 연결 */
    int fd;                  /* 모니터링 중인 파일 디스크립터 */
    uint32_t events;         /* 관심 있는 이벤트 */
    uint64_t user_data;      /* 사용자 데이터 */
    uint32_t revents;        /* 반환된 이벤트 */
};

/* epoll 인스턴스 */
struct epoll_instance {
    int epfd;                           /* epoll 파일 디스크립터 */
    struct rb_root items_tree;          /* 모니터링 중인 fd의 RB 트리 */
    struct epoll_item *ready_list;      /* 준비된 아이템 리스트 */
    int num_items;                      /* 모니터링 중인 아이템 수 */
    int in_use;                         /* 이 인스턴스가 사용 중인가? */
};

/* 전역 epoll 인스턴스들 */
struct epoll_instances {
    struct epoll_instance instances[MAX_EPOLL_INSTANCES];
};

/* epoll 서브시스템 초기화 */
void epoll_init(void);

/* epoll 인스턴스 생성 */
int epoll_create(int size);

/* epoll 파일 디스크립터의 제어 인터페이스 */
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

/* epoll 파일 디스크립터에서 이벤트 대기 */
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

/* epoll 인스턴스 닫기 */
int epoll_close(int epfd);

/* 헬퍼: epoll 인스턴스 가져오기 */
struct epoll_instance *epoll_get_instance(int epfd);

/* 헬퍼: RB 트리에서 아이템 찾기 */
struct epoll_item *epoll_find_item(struct epoll_instance *epi, int fd);

/* 헬퍼: 모든 모니터링 중인 fd를 폴링하고 준비 리스트 업데이트 */
void epoll_poll_fds(struct epoll_instance *epi);

/* 전역 epoll 인스턴스들 */
extern struct epoll_instances global_epoll;
