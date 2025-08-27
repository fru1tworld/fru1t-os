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

extern struct process processes[MAX_PROCESSES];
extern struct process *current_proc;

void scheduler_init(void);
void schedule(void);
struct process *create_process(void (*entry_point)(void));
void yield(void);
void process_exit(void);
void context_switch(struct process *prev, struct process *next);

#define HEAP_SIZE (1024 * 1024)
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
void cmd_netstat(void);
void cmd_ping(char *target);

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

#define ETHERNET_FRAME_SIZE 1518
#define IP_PACKET_SIZE 1500
#define TCP_SEGMENT_SIZE 1460
#define HTTP_BUFFER_SIZE 2048
#define MAX_CONNECTIONS 8

struct ethernet_header {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed));

struct ip_header {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed));

struct tcp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset_flags;
    uint8_t flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed));

struct tcp_connection {
    uint32_t remote_ip;
    uint16_t remote_port;
    uint16_t local_port;
    uint32_t seq_num;
    uint32_t ack_num;
    int state;
    char buffer[TCP_SEGMENT_SIZE];
    int buffer_len;
    int is_active;
};

struct http_request {
    char method[8];
    char path[256];
    char version[16];
    char headers[1024];
    char body[1024];
    int content_length;
};

struct http_response {
    int status_code;
    char status_text[32];
    char headers[1024];
    char body[2048];
    int body_length;
};

#define TCP_CLOSED 0
#define TCP_LISTEN 1
#define TCP_SYN_SENT 2
#define TCP_SYN_RECEIVED 3
#define TCP_ESTABLISHED 4
#define TCP_FIN_WAIT1 5
#define TCP_FIN_WAIT2 6
#define TCP_CLOSE_WAIT 7
#define TCP_CLOSING 8
#define TCP_LAST_ACK 9
#define TCP_TIME_WAIT 10

#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_URG 0x20

void network_init(void);
void ethernet_send(const uint8_t *dest_mac, uint16_t ethertype, const void *payload, size_t len);
void ip_send(uint32_t dest_ip, uint8_t protocol, const void *payload, size_t len);
void tcp_send(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port, uint32_t seq, uint32_t ack, uint8_t flags, const void *data, size_t len);
int tcp_listen(uint16_t port);
struct tcp_connection *tcp_accept(uint16_t port);
void tcp_close(struct tcp_connection *conn);
void http_init(void);
void http_handle_request(struct tcp_connection *conn, const char *request);
void http_send_response(struct tcp_connection *conn, struct http_response *response);
void process_network_packets(void);
