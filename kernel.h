#pragma once
#include "common.h"

#define NULL ((void*)0)

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;
typedef uint32_t uintptr_t;

#define PAGE_SIZE 4096

typedef uint32_t paddr_t;
typedef uint32_t vaddr_t;
typedef uint32_t size_t;

#define SCAUSE_ECALL 8
#define SCAUSE_INTERRUPT 0x80000000
#define SCAUSE_EXTERNAL_INTERRUPT 9
#define SCAUSE_TIMER_INTERRUPT 5

#define UART_BASE 0x10000000
#define UART_RHR 0
#define UART_THR 0
#define UART_IER 1
#define UART_IIR 2
#define UART_LCR 3
#define UART_LSR 5

#define UART_LSR_RX_READY (1 << 0)
#define UART_IER_RX_ENABLE (1 << 0)

#define PROC_UNUSED   0
#define PROC_READY    1
#define PROC_RUNNING  2
#define PROC_BLOCKED  3

#define MAX_PROCESSES 8
#define STACK_SIZE 8192
#define TIME_SLICE 10

struct process {
    int pid;
    int state;
    vaddr_t sp;
    uint32_t *page_table; 
    uint8_t stack[STACK_SIZE];
    struct trap_frame *trap_frame;
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

extern struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid);
extern void switch_to_user(uint32_t pc, uint32_t sp, uint32_t satp);
extern uint32_t read_csr_satp(void);
extern void write_csr_satp(uint32_t value);
extern uint32_t read_csr_scause(void);
extern uint32_t read_csr_stval(void);
extern uint32_t read_csr_sepc(void);
extern void write_csr_sepc(uint32_t value);
extern void switch_context(uint32_t **old_sp, uint32_t *new_sp);
extern void enable_interrupts(void);
extern void wait_for_interrupt(void);

#define READ_CSR(reg) read_csr_##reg()
#define WRITE_CSR(reg, value) write_csr_##reg(value)

#define PANIC(fmt, ...)                                                        \
    do {                                                                       \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
        while (1) {}                                                           \
    } while (0)

void *memset(void *s, int c, size_t n);
void handle_syscall(struct trap_frame *f);

extern struct process processes[MAX_PROCESSES];
extern struct process *current_proc;

void scheduler_init(void);
void schedule(void);
struct process *create_process(void (*entry_point)(void));
void yield(void);
void process_exit(void);
void context_switch(struct process *prev, struct process *next);

#define HEAP_SIZE (4 * 1024 * 1024)
#define BLOCK_SIZE 32
#define NUM_BLOCKS (HEAP_SIZE / BLOCK_SIZE)

struct mem_block {
    int is_free;
    size_t size;
    struct mem_block *next;
};

void memory_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void print_memory_stats(void);

#define MAX_FILES 32
#define MAX_FILENAME 64
#define MAX_FILESIZE 1024

struct file {
    char name[MAX_FILENAME];
    uint8_t *data;
    size_t size;
    int is_used;
};

struct filesystem {
    struct file files[MAX_FILES];
    int file_count;
};

void fs_init(void);
int fs_create(const char *filename, size_t size);
int fs_write(const char *filename, const void *data, size_t size);
int fs_read(const char *filename, void *buffer, size_t size);
int fs_delete(const char *filename);
void fs_list(void);
int fs_exists(const char *filename);

#define SHELL_BUFFER_SIZE 256
#define MAX_ARGS 10

struct shell_state {
    char input_buffer[SHELL_BUFFER_SIZE];
    int buffer_pos;
    int running;
};

void shell_init(void);
void shell_run(void);
void shell_parse_command(const char *input);
void shell_execute_command(const char *cmd, char *args[], int argc);
int shell_tokenize(const char *input, char *args[], int max_args);
void shell_print_prompt(void);

void cmd_help(void);
void cmd_ls(void);
void cmd_cat(char *filename);
void cmd_create(char *filename, char *size_str);
void cmd_delete(char *filename);
void cmd_memstat(void);
void cmd_clear(void);
void cmd_echo(char *args[], int argc);

#define INPUT_BUFFER_SIZE 256

struct input_buffer {
    char buffer[INPUT_BUFFER_SIZE];
    int write_pos;
    int read_pos;
    int count;
};

void uart_init(void);
void uart_enable_interrupts(void);
char uart_getchar(void);
void uart_putchar(char c);
int uart_rx_ready(void);
void handle_uart_interrupt(void);
void input_buffer_init(void);
void input_buffer_put(char c);
char input_buffer_get(void);
int input_buffer_available(void);
char getchar_blocking(void);

