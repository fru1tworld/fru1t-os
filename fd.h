#pragma once
#include "kernel.h"

/* epoll을 위한 파일 디스크립터 추상화 */

#define MAX_FDS 64
#define FD_TYPE_UNUSED 0
#define FD_TYPE_FILE   1
#define FD_TYPE_UART   2
#define FD_TYPE_PIPE   3
#define FD_TYPE_SOCKET 4

/* 파일 디스크립터 플래그 */
#define FD_READABLE  (1 << 0)
#define FD_WRITABLE  (1 << 1)
#define FD_ERROR     (1 << 2)
#define FD_HANGUP    (1 << 3)

/* 파일 디스크립터 연산 */
struct fd_ops {
    int (*read)(void *ctx, void *buf, size_t count);
    int (*write)(void *ctx, const void *buf, size_t count);
    int (*poll)(void *ctx);  /* FD_READABLE/FD_WRITABLE 등의 플래그 반환 */
    void (*close)(void *ctx);
};

/* 파일 디스크립터 구조체 */
struct fd {
    int fd_num;              /* 파일 디스크립터 번호 */
    int type;                /* FD_TYPE_* */
    int flags;               /* 현재 상태 플래그 */
    void *context;           /* 타입별 컨텍스트 */
    struct fd_ops *ops;      /* 연산들 */
    int ref_count;           /* 참조 카운트 */
};

/* 파일 디스크립터 테이블 (프로세스별이지만 간단함을 위해 전역 사용) */
struct fd_table {
    struct fd fds[MAX_FDS];
    int next_fd;
};

/* 파일 디스크립터 서브시스템 초기화 */
void fd_init(void);

/* 새 파일 디스크립터 할당 */
int fd_alloc(int type, void *context, struct fd_ops *ops);

/* 파일 디스크립터 구조체 가져오기 */
struct fd *fd_get(int fd_num);

/* 파일 디스크립터 닫기 */
int fd_close(int fd_num);

/* 이벤트에 대해 파일 디스크립터 폴링 */
int fd_poll(int fd_num);

/* 파일 디스크립터 플래그 업데이트 */
void fd_update_flags(int fd_num, int flags);

/* UART 전용 파일 디스크립터 연산 */
extern struct fd_ops uart_fd_ops;

/* 전역 파일 디스크립터 테이블 */
extern struct fd_table global_fd_table;
