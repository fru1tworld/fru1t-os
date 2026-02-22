#pragma once
#include "kernel.h"
#include "rbtree.h"

/* 완전 공정 스케줄러(CFS) 구현 */

#define CFS_ENABLED 1
#define NICE_0_LOAD 1024
#define MIN_GRANULARITY 1000000  /* 마이크로초 단위 1ms */
#define TARGET_LATENCY 6000000   /* 마이크로초 단위 6ms */

/* 프로세스별 스케줄링 엔티티 */
struct sched_entity {
    struct rb_node run_node;     /* RB 트리 노드 */
    uint64_t vruntime;           /* 나노초 단위 가상 실행시간 */
    uint64_t exec_start;         /* 프로세스가 실행을 시작한 시간 */
    uint64_t sum_exec_runtime;   /* 총 실행 시간 */
    uint32_t weight;             /* 우선순위 가중치 (높을수록 더 많은 CPU) */
    int on_rq;                   /* 이 엔티티가 실행 큐에 있는가? */
};

/* CFS 실행 큐 */
struct cfs_rq {
    struct rb_root tasks_timeline;  /* 실행 가능한 태스크의 RB 트리 */
    struct rb_node *rb_leftmost;    /* 가장 왼쪽(최소 vruntime) 노드 */
    uint64_t min_vruntime;          /* 트리의 최소 vruntime */
    uint32_t nr_running;            /* 실행 가능한 태스크 수 */
    uint64_t total_weight;          /* 모든 태스크 가중치의 합 */
};

/* CFS 지원이 추가된 확장 프로세스 구조체 */
struct cfs_process {
    struct process base;         /* 원본 프로세스 구조체 */
    struct sched_entity se;      /* 스케줄링 엔티티 */
    int nice;                    /* Nice 값 (-20에서 19) */
};

/* CFS 스케줄러 초기화 */
void cfs_init(void);

/* CFS 실행 큐에 프로세스 추가 */
void cfs_enqueue_task(struct cfs_process *proc);

/* CFS 실행 큐에서 프로세스 제거 */
void cfs_dequeue_task(struct cfs_process *proc);

/* 다음에 실행할 태스크 선택 */
struct cfs_process *cfs_pick_next_task(void);

/* 현재 태스크의 실행시간 업데이트 */
void cfs_update_curr(struct cfs_process *curr);

/* 현재 태스크가 선점되어야 하는지 확인 */
int cfs_check_preempt_curr(struct cfs_process *curr, struct cfs_process *new);

/* CFS 스케줄링 틱 */
void cfs_scheduler_tick(void);

/* CFS 프로세스 생성 */
struct cfs_process *cfs_create_process(void (*entry_point)(void), int nice);

/* nice 값을 가중치로 변환 */
uint32_t nice_to_weight(int nice);

/* vruntime 델타 계산 */
uint64_t calc_delta_fair(uint64_t delta, struct sched_entity *se);

/* 전역 CFS 실행 큐 */
extern struct cfs_rq cfs_runqueue;
extern struct cfs_process *cfs_current;
