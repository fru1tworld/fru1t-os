#pragma once
#include "kernel.h"
#include "rbtree.h"

/* Completely Fair Scheduler (CFS) implementation */

#define CFS_ENABLED 1
#define NICE_0_LOAD 1024
#define MIN_GRANULARITY 1000000  /* 1ms in microseconds */
#define TARGET_LATENCY 6000000   /* 6ms in microseconds */

/* Per-process scheduling entity */
struct sched_entity {
    struct rb_node run_node;     /* RB-tree node */
    uint64_t vruntime;           /* Virtual runtime in nanoseconds */
    uint64_t exec_start;         /* When process started executing */
    uint64_t sum_exec_runtime;   /* Total execution time */
    uint32_t weight;             /* Priority weight (higher = more CPU) */
    int on_rq;                   /* Is this entity on the runqueue? */
};

/* CFS run queue */
struct cfs_rq {
    struct rb_root tasks_timeline;  /* RB-tree of runnable tasks */
    struct rb_node *rb_leftmost;    /* Leftmost (min vruntime) node */
    uint64_t min_vruntime;          /* Minimum vruntime in the tree */
    uint32_t nr_running;            /* Number of runnable tasks */
    uint64_t total_weight;          /* Sum of all task weights */
};

/* Extended process structure with CFS support */
struct cfs_process {
    struct process base;         /* Original process structure */
    struct sched_entity se;      /* Scheduling entity */
    int nice;                    /* Nice value (-20 to 19) */
};

/* Initialize CFS scheduler */
void cfs_init(void);

/* Add a process to CFS runqueue */
void cfs_enqueue_task(struct cfs_process *proc);

/* Remove a process from CFS runqueue */
void cfs_dequeue_task(struct cfs_process *proc);

/* Pick next task to run */
struct cfs_process *cfs_pick_next_task(void);

/* Update current task's runtime */
void cfs_update_curr(struct cfs_process *curr);

/* Check if current task should be preempted */
int cfs_check_preempt_curr(struct cfs_process *curr, struct cfs_process *new);

/* CFS scheduling tick */
void cfs_scheduler_tick(void);

/* Create a CFS process */
struct cfs_process *cfs_create_process(void (*entry_point)(void), int nice);

/* Convert nice value to weight */
uint32_t nice_to_weight(int nice);

/* Calculate vruntime delta */
uint64_t calc_delta_fair(uint64_t delta, struct sched_entity *se);

/* Global CFS runqueue */
extern struct cfs_rq cfs_runqueue;
extern struct cfs_process *cfs_current;
