#pragma once
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

#define PAGE_SIZE 4096

typedef uint32_t paddr_t;
typedef uint32_t vaddr_t;
typedef uint32_t size_t;

#define SCAUSE_ECALL 8

struct process {
    int pid;
    int state;
    vaddr_t sp;
    uint32_t *page_table; 
    uint8_t stack[8192];
};

struct trap_frame {
    uint32_t ra, gp, tp, t0, t1, t2, t3, t4, t5, t6;
    uint32_t a0, a1, a2, a3, a4, a5, a6, a7;
    uint32_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    uint32_t sp;
};

struct sbiret {
    long error;
    long value;
};

#define READ_CSR(reg)                                                          \
    ({                                                                         \
        unsigned long __tmp;                                                   \
        __asm__ __volatile__("csrr %0, " #reg : "=r"(__tmp));                  \
        __tmp;                                                                 \
    })

#define WRITE_CSR(reg, value)                                                  \
    do {                                                                       \
        uint32_t __tmp = (value);                                              \
        __asm__ __volatile__("csrw " #reg ", %0" ::"r"(__tmp));                \
    } while (0)

#define PANIC(fmt, ...)                                                        \
    do {                                                                       \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
        while (1) {}                                                           \
    } while (0)

void *memset(void *s, int c, size_t n);
void handle_syscall(struct trap_frame *f);
