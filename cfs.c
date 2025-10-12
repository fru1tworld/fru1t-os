#include "cfs.h"
#include "common.h"

/* 전역 CFS 실행 큐 */
struct cfs_rq cfs_runqueue;
struct cfs_process *cfs_current = NULL;

/* CFS 프로세스 배열 */
static struct cfs_process cfs_processes[MAX_PROCESSES];

/* Nice 값을 가중치로 변환하는 테이블 (Linux 커널 값) */
static const uint32_t prio_to_weight[40] = {
    /* -20 */ 88761, 71755, 56483, 46273, 36291,
    /* -15 */ 29154, 23254, 18705, 14949, 11916,
    /* -10 */ 9548, 7620, 6100, 4904, 3906,
    /*  -5 */ 3121, 2501, 1991, 1586, 1277,
    /*   0 */ 1024, 820, 655, 526, 423,
    /*   5 */ 335, 272, 215, 172, 137,
    /*  10 */ 110, 87, 70, 56, 45,
    /*  15 */ 36, 29, 23, 18, 15,
};

/* 나노초 단위로 현재 시간 가져오기 (간소화됨) */
static uint64_t get_time_ns(void) {
    /* 실제 시스템에서는 하드웨어 타이머를 읽음 */
    static uint64_t counter = 0;
    counter += 1000000; /* 1ms씩 증가 */
    return counter;
}

/* nice 값을 가중치로 변환 */
uint32_t nice_to_weight(int nice) {
    if (nice < -20) nice = -20;
    if (nice > 19) nice = 19;
    return prio_to_weight[nice + 20];
}

/* 가중치 스케일링을 적용한 델타 계산 (32비트용 간소화) */
uint64_t calc_delta_fair(uint64_t delta, struct sched_entity *se) {
    /* 간소화: 32비트 플랫폼에서 64비트 나눗셈 피하기 */
    /* 가능하면 32비트 산술 연산 사용 */
    if (se->weight != NICE_0_LOAD && delta < 0xFFFFFFFF) {
        uint32_t delta32 = (uint32_t)delta;
        uint32_t scaled = (delta32 * NICE_0_LOAD) / se->weight;
        return (uint64_t)scaled;
    }
    return delta;
}

/* 최소 vruntime 업데이트 */
static void update_min_vruntime(struct cfs_rq *cfs_rq) {
    uint64_t vruntime = cfs_rq->min_vruntime;

    if (cfs_current) {
        vruntime = cfs_current->se.vruntime;
    }

    if (cfs_rq->rb_leftmost) {
        struct sched_entity *se = rb_entry(cfs_rq->rb_leftmost,
                                           struct sched_entity, run_node);
        if (!cfs_current) {
            vruntime = se->vruntime;
        } else {
            vruntime = vruntime < se->vruntime ? vruntime : se->vruntime;
        }
    }

    cfs_rq->min_vruntime = vruntime > cfs_rq->min_vruntime ?
                           vruntime : cfs_rq->min_vruntime;
}

/* CFS 스케줄러 초기화 */
void cfs_init(void) {
    cfs_runqueue.tasks_timeline = RB_ROOT;
    cfs_runqueue.rb_leftmost = NULL;
    cfs_runqueue.min_vruntime = 0;
    cfs_runqueue.nr_running = 0;
    cfs_runqueue.total_weight = 0;
    cfs_current = NULL;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        cfs_processes[i].base.pid = i;
        cfs_processes[i].base.state = PROC_UNUSED;
        cfs_processes[i].se.vruntime = 0;
        cfs_processes[i].se.on_rq = 0;
        cfs_processes[i].nice = 0;
        RB_CLEAR_NODE(&cfs_processes[i].se.run_node);
    }

    printf("CFS scheduler initialized\n");
}

/* RB 트리에 태스크 삽입 */
void cfs_enqueue_task(struct cfs_process *proc) {
    struct cfs_rq *cfs_rq = &cfs_runqueue;
    struct sched_entity *se = &proc->se;
    struct rb_node **link = &cfs_rq->tasks_timeline.rb_node;
    struct rb_node *parent = NULL;
    int leftmost = 1;

    if (se->on_rq) {
        return;
    }

    /* 삽입 위치 찾기 */
    while (*link) {
        struct sched_entity *entry;
        parent = *link;
        entry = rb_entry(parent, struct sched_entity, run_node);

        if (se->vruntime < entry->vruntime) {
            link = &parent->left;
        } else {
            link = &parent->right;
            leftmost = 0;
        }
    }

    /* 필요시 leftmost 포인터 업데이트 */
    if (leftmost) {
        cfs_rq->rb_leftmost = &se->run_node;
    }

    /* RB 트리에 삽입 */
    se->run_node.parent = parent;
    se->run_node.left = NULL;
    se->run_node.right = NULL;
    se->run_node.color = RB_RED;

    if (parent) {
        if (link == &parent->left)
            parent->left = &se->run_node;
        else
            parent->right = &se->run_node;
    } else {
        cfs_rq->tasks_timeline.rb_node = &se->run_node;
    }

    rb_insert_color(&se->run_node, &cfs_rq->tasks_timeline);

    se->on_rq = 1;
    cfs_rq->nr_running++;
    cfs_rq->total_weight += se->weight;

    proc->base.state = PROC_READY;

    printf("CFS: Enqueued process %d (vruntime=%llu, weight=%u)\n",
           proc->base.pid, se->vruntime, se->weight);
}

/* RB 트리에서 태스크 제거 */
void cfs_dequeue_task(struct cfs_process *proc) {
    struct cfs_rq *cfs_rq = &cfs_runqueue;
    struct sched_entity *se = &proc->se;

    if (!se->on_rq) {
        return;
    }

    /* 필요시 leftmost 업데이트 */
    if (cfs_rq->rb_leftmost == &se->run_node) {
        cfs_rq->rb_leftmost = rb_next(&se->run_node);
    }

    /* RB 트리에서 제거 */
    rb_erase(&se->run_node, &cfs_rq->tasks_timeline);
    RB_CLEAR_NODE(&se->run_node);

    se->on_rq = 0;
    cfs_rq->nr_running--;
    cfs_rq->total_weight -= se->weight;

    update_min_vruntime(cfs_rq);

    printf("CFS: Dequeued process %d\n", proc->base.pid);
}

/* 최소 vruntime을 가진 다음 태스크 선택 */
struct cfs_process *cfs_pick_next_task(void) {
    struct cfs_rq *cfs_rq = &cfs_runqueue;
    struct sched_entity *se;

    if (!cfs_rq->rb_leftmost) {
        return NULL;
    }

    se = rb_entry(cfs_rq->rb_leftmost, struct sched_entity, run_node);
    return rb_entry(&se->run_node, struct cfs_process, se.run_node);
}

/* 현재 태스크의 실행시간 업데이트 */
void cfs_update_curr(struct cfs_process *curr) {
    struct sched_entity *se = &curr->se;
    uint64_t now = get_time_ns();
    uint64_t delta_exec;

    if (!se->exec_start) {
        se->exec_start = now;
        return;
    }

    delta_exec = now - se->exec_start;
    se->exec_start = now;
    se->sum_exec_runtime += delta_exec;

    /* 가중치 스케일링을 적용하여 vruntime 업데이트 */
    se->vruntime += calc_delta_fair(delta_exec, se);

    update_min_vruntime(&cfs_runqueue);

    printf("CFS: Updated process %d vruntime=%llu (delta=%llu)\n",
           curr->base.pid, se->vruntime, delta_exec);
}

/* 현재 태스크가 선점되어야 하는지 확인 */
int cfs_check_preempt_curr(struct cfs_process *curr, struct cfs_process *new) {
    uint64_t gran = MIN_GRANULARITY;
    uint64_t vdiff = curr->se.vruntime - new->se.vruntime;

    /* vruntime 차이가 granularity를 초과하면 선점 */
    return vdiff > gran;
}

/* 메인 CFS 스케줄러 함수 */
void cfs_scheduler_tick(void) {
    struct cfs_process *curr = cfs_current;
    struct cfs_process *next;

    if (!curr) {
        /* 현재 태스크 없음, 다음 태스크 선택 */
        next = cfs_pick_next_task();
        if (next) {
            cfs_dequeue_task(next);
            next->base.state = PROC_RUNNING;
            next->se.exec_start = get_time_ns();
            cfs_current = next;
            printf("CFS: Scheduled process %d (vruntime=%llu)\n",
                   next->base.pid, next->se.vruntime);
        }
        return;
    }

    /* 현재 태스크의 실행시간 업데이트 */
    cfs_update_curr(curr);

    /* 선점이 필요한지 확인 */
    next = cfs_pick_next_task();
    if (next && cfs_check_preempt_curr(curr, next)) {
        /* 컨텍스트 스위치 필요 */
        printf("CFS: Preempting process %d with process %d\n",
               curr->base.pid, next->base.pid);

        /* 현재 태스크를 실행 큐에 다시 넣기 */
        curr->base.state = PROC_READY;
        curr->se.exec_start = 0;
        cfs_enqueue_task(curr);

        /* 다음 태스크 스케줄 */
        cfs_dequeue_task(next);
        next->base.state = PROC_RUNNING;
        next->se.exec_start = get_time_ns();
        cfs_current = next;

        /* 실제 시스템에서는 여기서 context_switch(curr, next) 수행 */
    }
}

/* CFS 프로세스 생성 */
struct cfs_process *cfs_create_process(void (*entry_point)(void), int nice) {
    struct cfs_process *proc = NULL;

    /* 빈 슬롯 찾기 */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (cfs_processes[i].base.state == PROC_UNUSED) {
            proc = &cfs_processes[i];
            break;
        }
    }

    if (!proc) {
        printf("CFS: No free process slots\n");
        return NULL;
    }

    /* 기본 프로세스 초기화 */
    proc->base.state = PROC_READY;
    proc->base.sp = (vaddr_t)&proc->base.stack[STACK_SIZE - sizeof(struct trap_frame)];
    proc->base.trap_frame = (struct trap_frame *)proc->base.sp;

    memset(proc->base.trap_frame, 0, sizeof(struct trap_frame));
    proc->base.trap_frame->ra = (uint32_t)entry_point;
    proc->base.trap_frame->sp = (uint32_t)&proc->base.stack[STACK_SIZE - 8];

    /* 스케줄링 엔티티 초기화 */
    proc->nice = nice;
    proc->se.weight = nice_to_weight(nice);
    proc->se.vruntime = cfs_runqueue.min_vruntime;
    proc->se.exec_start = 0;
    proc->se.sum_exec_runtime = 0;
    proc->se.on_rq = 0;
    RB_CLEAR_NODE(&proc->se.run_node);

    printf("CFS: Created process %d (nice=%d, weight=%u)\n",
           proc->base.pid, nice, proc->se.weight);

    /* 실행 큐에 추가 */
    cfs_enqueue_task(proc);

    return proc;
}
