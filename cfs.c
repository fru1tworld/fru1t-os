#include "cfs.h"
#include "common.h"

/* Global CFS runqueue */
struct cfs_rq cfs_runqueue;
struct cfs_process *cfs_current = NULL;

/* Array of CFS processes */
static struct cfs_process cfs_processes[MAX_PROCESSES];

/* Nice to weight conversion table (Linux kernel values) */
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

/* Get current time in nanoseconds (simplified) */
static uint64_t get_time_ns(void) {
    /* In a real system, this would read hardware timer */
    static uint64_t counter = 0;
    counter += 1000000; /* Increment by 1ms */
    return counter;
}

/* Convert nice value to weight */
uint32_t nice_to_weight(int nice) {
    if (nice < -20) nice = -20;
    if (nice > 19) nice = 19;
    return prio_to_weight[nice + 20];
}

/* Calculate delta with weight scaling (simplified for 32-bit) */
uint64_t calc_delta_fair(uint64_t delta, struct sched_entity *se) {
    /* Simplified: avoid 64-bit division on 32-bit platform */
    /* Use 32-bit arithmetic when possible */
    if (se->weight != NICE_0_LOAD && delta < 0xFFFFFFFF) {
        uint32_t delta32 = (uint32_t)delta;
        uint32_t scaled = (delta32 * NICE_0_LOAD) / se->weight;
        return (uint64_t)scaled;
    }
    return delta;
}

/* Update minimum vruntime */
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

/* Initialize CFS scheduler */
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

/* Enqueue task into RB-tree */
void cfs_enqueue_task(struct cfs_process *proc) {
    struct cfs_rq *cfs_rq = &cfs_runqueue;
    struct sched_entity *se = &proc->se;
    struct rb_node **link = &cfs_rq->tasks_timeline.rb_node;
    struct rb_node *parent = NULL;
    int leftmost = 1;

    if (se->on_rq) {
        return;
    }

    /* Find insertion point */
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

    /* Update leftmost pointer if necessary */
    if (leftmost) {
        cfs_rq->rb_leftmost = &se->run_node;
    }

    /* Insert into RB-tree */
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

/* Dequeue task from RB-tree */
void cfs_dequeue_task(struct cfs_process *proc) {
    struct cfs_rq *cfs_rq = &cfs_runqueue;
    struct sched_entity *se = &proc->se;

    if (!se->on_rq) {
        return;
    }

    /* Update leftmost if necessary */
    if (cfs_rq->rb_leftmost == &se->run_node) {
        cfs_rq->rb_leftmost = rb_next(&se->run_node);
    }

    /* Remove from RB-tree */
    rb_erase(&se->run_node, &cfs_rq->tasks_timeline);
    RB_CLEAR_NODE(&se->run_node);

    se->on_rq = 0;
    cfs_rq->nr_running--;
    cfs_rq->total_weight -= se->weight;

    update_min_vruntime(cfs_rq);

    printf("CFS: Dequeued process %d\n", proc->base.pid);
}

/* Pick next task with minimum vruntime */
struct cfs_process *cfs_pick_next_task(void) {
    struct cfs_rq *cfs_rq = &cfs_runqueue;
    struct sched_entity *se;

    if (!cfs_rq->rb_leftmost) {
        return NULL;
    }

    se = rb_entry(cfs_rq->rb_leftmost, struct sched_entity, run_node);
    return rb_entry(&se->run_node, struct cfs_process, se.run_node);
}

/* Update current task's runtime */
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

    /* Update vruntime with weight scaling */
    se->vruntime += calc_delta_fair(delta_exec, se);

    update_min_vruntime(&cfs_runqueue);

    printf("CFS: Updated process %d vruntime=%llu (delta=%llu)\n",
           curr->base.pid, se->vruntime, delta_exec);
}

/* Check if current should be preempted */
int cfs_check_preempt_curr(struct cfs_process *curr, struct cfs_process *new) {
    uint64_t gran = MIN_GRANULARITY;
    uint64_t vdiff = curr->se.vruntime - new->se.vruntime;

    /* Preempt if vruntime difference exceeds granularity */
    return vdiff > gran;
}

/* Main CFS scheduler function */
void cfs_scheduler_tick(void) {
    struct cfs_process *curr = cfs_current;
    struct cfs_process *next;

    if (!curr) {
        /* No current task, pick next */
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

    /* Update current task's runtime */
    cfs_update_curr(curr);

    /* Check if we should preempt */
    next = cfs_pick_next_task();
    if (next && cfs_check_preempt_curr(curr, next)) {
        /* Context switch needed */
        printf("CFS: Preempting process %d with process %d\n",
               curr->base.pid, next->base.pid);

        /* Put current back on runqueue */
        curr->base.state = PROC_READY;
        curr->se.exec_start = 0;
        cfs_enqueue_task(curr);

        /* Schedule next */
        cfs_dequeue_task(next);
        next->base.state = PROC_RUNNING;
        next->se.exec_start = get_time_ns();
        cfs_current = next;

        /* In a real system, do context_switch(curr, next) here */
    }
}

/* Create a CFS process */
struct cfs_process *cfs_create_process(void (*entry_point)(void), int nice) {
    struct cfs_process *proc = NULL;

    /* Find free slot */
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

    /* Initialize base process */
    proc->base.state = PROC_READY;
    proc->base.sp = (vaddr_t)&proc->base.stack[STACK_SIZE - sizeof(struct trap_frame)];
    proc->base.trap_frame = (struct trap_frame *)proc->base.sp;

    memset(proc->base.trap_frame, 0, sizeof(struct trap_frame));
    proc->base.trap_frame->ra = (uint32_t)entry_point;
    proc->base.trap_frame->sp = (uint32_t)&proc->base.stack[STACK_SIZE - 8];

    /* Initialize scheduling entity */
    proc->nice = nice;
    proc->se.weight = nice_to_weight(nice);
    proc->se.vruntime = cfs_runqueue.min_vruntime;
    proc->se.exec_start = 0;
    proc->se.sum_exec_runtime = 0;
    proc->se.on_rq = 0;
    RB_CLEAR_NODE(&proc->se.run_node);

    printf("CFS: Created process %d (nice=%d, weight=%u)\n",
           proc->base.pid, nice, proc->se.weight);

    /* Add to runqueue */
    cfs_enqueue_task(proc);

    return proc;
}
