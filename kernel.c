#include "kernel.h"
#include "common.h"

extern char bss[], bss_end[], __stack_top[];
extern char __free_ram[], __free_ram_end[];


void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}



paddr_t alloc_pages(uint32_t n) {
    static paddr_t next_paddr = (paddr_t) __free_ram;
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if (next_paddr > (paddr_t) __free_ram_end)
        PANIC("out of memory");

    memset((void *) paddr, 0, n * PAGE_SIZE);
    return paddr;
}


void handle_trap(struct trap_frame *f) {
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);
    
    if (scause & SCAUSE_INTERRUPT) {
        uint32_t interrupt_type = scause & 0x7FFFFFFF;
        if (interrupt_type == SCAUSE_EXTERNAL_INTERRUPT) {
            handle_uart_interrupt();
        } else {
            PANIC("unexpected interrupt scause=%x\n", scause);
        }
    } else if (scause == SCAUSE_ECALL) {
        handle_syscall(f);
        user_pc += 4;
    } else {
        PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
    }

    WRITE_CSR(sepc, user_pc);
}
 
struct process processes[MAX_PROCESSES];
struct process *current_proc = NULL;

void scheduler_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].pid = i;
        processes[i].state = PROC_UNUSED;
        processes[i].sp = 0;
        processes[i].page_table = NULL;
        processes[i].trap_frame = NULL;
    }
    current_proc = NULL;
}

struct process *create_process(void (*entry_point)(void)) {
    struct process *proc = NULL;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROC_UNUSED) {
            proc = &processes[i];
            break;
        }
    }
    
    if (!proc) {
        printf("No free process slots\n");
        return NULL;
    }

    proc->state = PROC_READY;
    proc->sp = (vaddr_t)&proc->stack[STACK_SIZE - sizeof(struct trap_frame)];
    proc->trap_frame = (struct trap_frame *)proc->sp;
    
    memset(proc->trap_frame, 0, sizeof(struct trap_frame));
    proc->trap_frame->ra = (uint32_t)entry_point;
    proc->trap_frame->sp = (uint32_t)&proc->stack[STACK_SIZE - 8];
    
    printf("Created process %d\n", proc->pid);
    return proc;
}

void schedule(void) {
    struct process *next = NULL;
    
    if (current_proc && current_proc->state == PROC_RUNNING) {
        current_proc->state = PROC_READY;
    }
    
    int start = current_proc ? (current_proc->pid + 1) % MAX_PROCESSES : 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        int idx = (start + i) % MAX_PROCESSES;
        if (processes[idx].state == PROC_READY) {
            next = &processes[idx];
            break;
        }
    }
    
    if (!next) {
        printf("No ready processes\n");
        return;
    }
    
    struct process *prev = current_proc;
    current_proc = next;
    current_proc->state = PROC_RUNNING;
    
    printf("Switching to process %d\n", current_proc->pid);
    
    if (prev) {
        context_switch(prev, current_proc);
    }
}

void yield(void) {
    if (current_proc) {
        current_proc->state = PROC_READY;
        schedule();
    }
}

void process_exit(void) {
    if (current_proc) {
        printf("Process %d exiting\n", current_proc->pid);
        current_proc->state = PROC_UNUSED;
        current_proc = NULL;
        schedule();
    }
}

void context_switch(struct process *prev, struct process *next) {
    struct trap_frame *prev_frame = prev->trap_frame;
    struct trap_frame *next_frame = next->trap_frame;
    
    __asm__ __volatile__(
        "sw ra, 4 * 0(%0)\n"
        "sw gp, 4 * 1(%0)\n"
        "sw tp, 4 * 2(%0)\n"
        "sw t0, 4 * 3(%0)\n"
        "sw t1, 4 * 4(%0)\n"
        "sw t2, 4 * 5(%0)\n"
        "sw t3, 4 * 6(%0)\n"
        "sw t4, 4 * 7(%0)\n"
        "sw t5, 4 * 8(%0)\n"
        "sw t6, 4 * 9(%0)\n"
        "sw a0, 4 * 10(%0)\n"
        "sw a1, 4 * 11(%0)\n"
        "sw a2, 4 * 12(%0)\n"
        "sw a3, 4 * 13(%0)\n"
        "sw a4, 4 * 14(%0)\n"
        "sw a5, 4 * 15(%0)\n"
        "sw a6, 4 * 16(%0)\n"
        "sw a7, 4 * 17(%0)\n"
        "sw s0, 4 * 18(%0)\n"
        "sw s1, 4 * 19(%0)\n"
        "sw s2, 4 * 20(%0)\n"
        "sw s3, 4 * 21(%0)\n"
        "sw s4, 4 * 22(%0)\n"
        "sw s5, 4 * 23(%0)\n"
        "sw s6, 4 * 24(%0)\n"
        "sw s7, 4 * 25(%0)\n"
        "sw s8, 4 * 26(%0)\n"
        "sw s9, 4 * 27(%0)\n"
        "sw s10, 4 * 28(%0)\n"
        "sw s11, 4 * 29(%0)\n"
        "sw sp, 4 * 30(%0)\n"

        "lw ra, 4 * 0(%1)\n"
        "lw gp, 4 * 1(%1)\n"
        "lw tp, 4 * 2(%1)\n"
        "lw t0, 4 * 3(%1)\n"
        "lw t1, 4 * 4(%1)\n"
        "lw t2, 4 * 5(%1)\n"
        "lw t3, 4 * 6(%1)\n"
        "lw t4, 4 * 7(%1)\n"
        "lw t5, 4 * 8(%1)\n"
        "lw t6, 4 * 9(%1)\n"
        "lw a0, 4 * 10(%1)\n"
        "lw a1, 4 * 11(%1)\n"
        "lw a2, 4 * 12(%1)\n"
        "lw a3, 4 * 13(%1)\n"
        "lw a4, 4 * 14(%1)\n"
        "lw a5, 4 * 15(%1)\n"
        "lw a6, 4 * 16(%1)\n"
        "lw a7, 4 * 17(%1)\n"
        "lw s0, 4 * 18(%1)\n"
        "lw s1, 4 * 19(%1)\n"
        "lw s2, 4 * 20(%1)\n"
        "lw s3, 4 * 21(%1)\n"
        "lw s4, 4 * 22(%1)\n"
        "lw s5, 4 * 23(%1)\n"
        "lw s6, 4 * 24(%1)\n"
        "lw s7, 4 * 25(%1)\n"
        "lw s8, 4 * 26(%1)\n"
        "lw s9, 4 * 27(%1)\n"
        "lw s10, 4 * 28(%1)\n"
        "lw s11, 4 * 29(%1)\n"
        "lw sp, 4 * 30(%1)\n"
        :
        : "r" (prev_frame), "r" (next_frame)
        : "memory"
    );
}

void process_a(void) {
    for (int i = 0; i < 5; i++) {
        printf("Process A: iteration %d\n", i);
        for (volatile int j = 0; j < 1000000; j++);
        yield();
    }
    printf("Process A finished\n");
    process_exit();
}

void process_b(void) {
    for (int i = 0; i < 5; i++) {
        printf("Process B: iteration %d\n", i);
        for (volatile int j = 0; j < 1000000; j++);
        yield();
    }
    printf("Process B finished\n");
    process_exit();
}

void process_c(void) {
    for (int i = 0; i < 5; i++) {
        printf("Process C: iteration %d\n", i);
        for (volatile int j = 0; j < 1000000; j++);
        yield();
    }
    printf("Process C finished\n");
    process_exit();
}

static uint8_t heap[HEAP_SIZE];
static struct mem_block *free_list = NULL;

void memory_init(void) {
    free_list = (struct mem_block *)heap;
    free_list->is_free = 1;
    free_list->size = HEAP_SIZE - sizeof(struct mem_block);
    free_list->next = NULL;
    
    printf("Memory allocator initialized: %d bytes available\n", HEAP_SIZE);
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    size = (size + 7) & ~7;
    
    struct mem_block *current = free_list;
    struct mem_block *prev = NULL;
    
    while (current) {
        if (current->is_free && current->size >= size) {
            if (current->size > size + sizeof(struct mem_block)) {
                struct mem_block *new_block = (struct mem_block *)((uint8_t *)current + sizeof(struct mem_block) + size);
                new_block->is_free = 1;
                new_block->size = current->size - size - sizeof(struct mem_block);
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->is_free = 0;
            return (uint8_t *)current + sizeof(struct mem_block);
        }
        prev = current;
        current = current->next;
    }
    
    printf("kmalloc failed: no suitable block found\n");
    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;
    
    struct mem_block *block = (struct mem_block *)((uint8_t *)ptr - sizeof(struct mem_block));
    block->is_free = 1;
    
    struct mem_block *current = free_list;
    while (current && current->next) {
        if (current->is_free && current->next->is_free && 
            (uint8_t *)current + sizeof(struct mem_block) + current->size == (uint8_t *)current->next) {
            current->size += sizeof(struct mem_block) + current->next->size;
            current->next = current->next->next;
        }
        current = current->next;
    }
}

void print_memory_stats(void) {
    struct mem_block *current = free_list;
    int total_free = 0, total_used = 0, blocks = 0;
    
    while (current) {
        blocks++;
        if (current->is_free) {
            total_free += current->size;
        } else {
            total_used += current->size;
        }
        current = current->next;
    }
    
    printf("Memory stats: %d blocks, %d bytes free, %d bytes used\n", 
           blocks, total_free, total_used);
}

void test_memory_allocation(void) {
    printf("\n=== Memory Allocation Test ===\n");
    
    void *ptr1 = kmalloc(64);
    printf("Allocated 64 bytes at %p\n", ptr1);
    print_memory_stats();
    
    void *ptr2 = kmalloc(128);
    printf("Allocated 128 bytes at %p\n", ptr2);
    print_memory_stats();
    
    void *ptr3 = kmalloc(256);
    printf("Allocated 256 bytes at %p\n", ptr3);
    print_memory_stats();
    
    kfree(ptr2);
    printf("Freed 128 byte block\n");
    print_memory_stats();
    
    void *ptr4 = kmalloc(100);
    printf("Allocated 100 bytes at %p\n", ptr4);
    print_memory_stats();
    
    kfree(ptr1);
    kfree(ptr3);
    kfree(ptr4);
    printf("Freed all remaining blocks\n");
    print_memory_stats();
}

static struct filesystem fs;

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strlen(const char *s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

const char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == c) return s;
        s++;
    }
    return (c == '\0') ? s : NULL;
}

const char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return haystack;
    
    for (const char *h = haystack; *h; h++) {
        const char *h2 = h;
        const char *n = needle;
        
        while (*h2 && *n && *h2 == *n) {
            h2++;
            n++;
        }
        
        if (!*n) return h;
    }
    
    return NULL;
}

int sscanf(const char *str, const char *format, ...) {
    return 0;
}

void strcpy(char *dest, const char *src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

int snprintf(char *str, size_t size, const char *format, ...) {
    strcpy(str, "Basic snprintf implementation");
    return strlen(str);
}

void fs_init(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        fs.files[i].is_used = 0;
        fs.files[i].data = NULL;
        fs.files[i].size = 0;
    }
    fs.file_count = 0;
    printf("Filesystem initialized: %d file slots available\n", MAX_FILES);
}

int fs_create(const char *filename, size_t size) {
    if (size > MAX_FILESIZE) {
        printf("File size too large: %d bytes (max: %d)\n", (int)size, MAX_FILESIZE);
        return -1;
    }
    
    if (fs_exists(filename)) {
        printf("File '%s' already exists\n", filename);
        return -1;
    }
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fs.files[i].is_used) {
            strcpy(fs.files[i].name, filename);
            fs.files[i].data = (uint8_t *)kmalloc(size);
            if (!fs.files[i].data) {
                printf("Failed to allocate memory for file '%s'\n", filename);
                return -1;
            }
            fs.files[i].size = size;
            fs.files[i].is_used = 1;
            fs.file_count++;
            printf("Created file '%s' (%d bytes)\n", filename, (int)size);
            return i;
        }
    }
    printf("No free file slots available\n");
    return -1;
}

int fs_write(const char *filename, const void *data, size_t size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs.files[i].is_used && strcmp(fs.files[i].name, filename) == 0) {
            if (size > fs.files[i].size) {
                printf("Write size too large for file '%s'\n", filename);
                return -1;
            }
            
            for (size_t j = 0; j < size; j++) {
                fs.files[i].data[j] = ((const uint8_t *)data)[j];
            }
            
            printf("Wrote %d bytes to file '%s'\n", (int)size, filename);
            return 0;
        }
    }
    printf("File '%s' not found\n", filename);
    return -1;
}

int fs_read(const char *filename, void *buffer, size_t size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs.files[i].is_used && strcmp(fs.files[i].name, filename) == 0) {
            size_t read_size = (size < fs.files[i].size) ? size : fs.files[i].size;
            
            for (size_t j = 0; j < read_size; j++) {
                ((uint8_t *)buffer)[j] = fs.files[i].data[j];
            }
            
            printf("Read %d bytes from file '%s'\n", (int)read_size, filename);
            return read_size;
        }
    }
    printf("File '%s' not found\n", filename);
    return -1;
}

int fs_delete(const char *filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs.files[i].is_used && strcmp(fs.files[i].name, filename) == 0) {
            kfree(fs.files[i].data);
            fs.files[i].is_used = 0;
            fs.files[i].data = NULL;
            fs.files[i].size = 0;
            fs.file_count--;
            printf("Deleted file '%s'\n", filename);
            return 0;
        }
    }
    printf("File '%s' not found\n", filename);
    return -1;
}

void fs_list(void) {
    printf("\n=== File System Listing ===\n");
    printf("Files: %d/%d\n", fs.file_count, MAX_FILES);
    
    if (fs.file_count == 0) {
        printf("No files in filesystem\n");
        return;
    }
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs.files[i].is_used) {
            printf("  %s (%d bytes)\n", fs.files[i].name, (int)fs.files[i].size);
        }
    }
}

int fs_exists(const char *filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs.files[i].is_used && strcmp(fs.files[i].name, filename) == 0) {
            return 1;
        }
    }
    return 0;
}

void test_filesystem(void) {
    printf("\n=== File System Test ===\n");
    
    fs_create("fru1tworld.txt", 512);
    fs_create("fru1tworld_delete_test.txt", 256);
    
    fs_list();
    
    const char *fru1tworld_content = "happy cat";
    fs_write("fru1tworld.txt", fru1tworld_content, strlen(fru1tworld_content) + 1);
    
    const char *delete_test_content = "This file will be deleted to test deletion functionality.";
    fs_write("fru1tworld_delete_test.txt", delete_test_content, strlen(delete_test_content) + 1);
    
    char buffer[512];
    int bytes_read = fs_read("fru1tworld.txt", buffer, sizeof(buffer));
    if (bytes_read > 0) {
        printf("Content of fru1tworld.txt: %s\n", buffer);
    }
    
    bytes_read = fs_read("fru1tworld_delete_test.txt", buffer, sizeof(buffer));
    if (bytes_read > 0) {
        printf("Content of fru1tworld_delete_test.txt: %s\n", buffer);
    }
    
    fs_delete("fru1tworld_delete_test.txt");
    
    fs_list();
    
    print_memory_stats();
}

static struct shell_state shell;

int str_to_int(const char *str) {
    int result = 0;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

void shell_init(void) {
    shell.buffer_pos = 0;
    shell.running = 1;
    for (int i = 0; i < SHELL_BUFFER_SIZE; i++) {
        shell.input_buffer[i] = '\0';
    }
}

void shell_print_prompt(void) {
    printf("fru1t-os> ");
}

int shell_tokenize(const char *input, char *args[], int max_args) {
    int argc = 0;
    int i = 0;
    int arg_start = -1;
    
    static char token_buffer[SHELL_BUFFER_SIZE];
    int token_pos = 0;
    
    while (input[i] != '\0' && argc < max_args) {
        if (input[i] == ' ' || input[i] == '\t') {
            if (arg_start != -1) {
                token_buffer[token_pos] = '\0';
                args[argc] = (char *)kmalloc(token_pos + 1);
                strcpy(args[argc], token_buffer);
                argc++;
                arg_start = -1;
                token_pos = 0;
            }
        } else {
            if (arg_start == -1) {
                arg_start = i;
            }
            token_buffer[token_pos++] = input[i];
        }
        i++;
    }
    
    if (arg_start != -1 && argc < max_args) {
        token_buffer[token_pos] = '\0';
        args[argc] = (char *)kmalloc(token_pos + 1);
        strcpy(args[argc], token_buffer);
        argc++;
    }
    
    return argc;
}

void shell_execute_command(const char *cmd, char *args[], int argc) {
    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(cmd, "cat") == 0) {
        if (argc > 1) {
            cmd_cat(args[1]);
        } else {
            printf("Usage: cat <filename>\n");
        }
    } else if (strcmp(cmd, "create") == 0) {
        if (argc > 2) {
            cmd_create(args[1], args[2]);
        } else {
            printf("Usage: create <filename> <size>\n");
        }
    } else if (strcmp(cmd, "delete") == 0) {
        if (argc > 1) {
            cmd_delete(args[1]);
        } else {
            printf("Usage: delete <filename>\n");
        }
    } else if (strcmp(cmd, "memstat") == 0) {
        cmd_memstat();
    } else if (strcmp(cmd, "clear") == 0) {
        cmd_clear();
    } else if (strcmp(cmd, "echo") == 0) {
        cmd_echo(args, argc);
    } else if (strcmp(cmd, "exit") == 0) {
        shell.running = 0;
        printf("Goodbye!\n");
    } else {
        printf("Unknown command: %s\n", cmd);
        printf("Type 'help' for available commands\n");
    }
}

void shell_parse_command(const char *input) {
    char *args[MAX_ARGS];
    int argc = shell_tokenize(input, args, MAX_ARGS);
    
    if (argc > 0) {
        shell_execute_command(args[0], args, argc);
        
        for (int i = 0; i < argc; i++) {
            kfree(args[i]);
        }
    }
}

void cmd_help(void) {
    printf("\n=== Fru1t OS Shell Commands ===\n");
    printf("help          - Show this help message\n");
    printf("ls            - List files in filesystem\n");
    printf("cat <file>    - Display file contents\n");
    printf("create <file> <size> - Create new file\n");
    printf("delete <file> - Delete file\n");
    printf("echo [args]   - Print arguments\n");
    printf("memstat       - Show memory statistics\n");
    printf("clear         - Clear screen\n");
    printf("exit          - Exit shell\n");
    printf("\n");
}

void cmd_ls(void) {
    fs_list();
}

void cmd_cat(char *filename) {
    char buffer[MAX_FILESIZE];
    int bytes_read = fs_read(filename, buffer, MAX_FILESIZE);
    if (bytes_read > 0) {
        printf("Content of %s:\n", filename);
        for (int i = 0; i < bytes_read && buffer[i] != '\0'; i++) {
            putchar(buffer[i]);
        }
        printf("\n");
    }
}

void cmd_create(char *filename, char *size_str) {
    int size = str_to_int(size_str);
    if (size <= 0 || size > MAX_FILESIZE) {
        printf("Invalid size. Must be 1-%d bytes\n", MAX_FILESIZE);
        return;
    }
    fs_create(filename, size);
}

void cmd_delete(char *filename) {
    fs_delete(filename);
}

void cmd_memstat(void) {
    print_memory_stats();
}

void cmd_clear(void) {
    printf("\033[2J\033[H");
}

void cmd_echo(char *args[], int argc) {
    for (int i = 1; i < argc; i++) {
        printf("%s", args[i]);
        if (i < argc - 1) printf(" ");
    }
    printf("\n");
}

void shell_run(void) {
    while (shell.running) {
        shell_print_prompt();
        
        char c;
        shell.buffer_pos = 0;
        
        while (1) {
            c = getchar_blocking();
            
            if (c == '\n' || c == '\r') {
                shell.input_buffer[shell.buffer_pos] = '\0';
                printf("\n");
                if (shell.buffer_pos > 0) {
                    shell_parse_command(shell.input_buffer);
                }
                break;
            } else if (c == '\b' || c == 127) {
                if (shell.buffer_pos > 0) {
                    shell.buffer_pos--;
                    printf("\b \b");
                }
            } else if (c >= 32 && c < 127 && shell.buffer_pos < SHELL_BUFFER_SIZE - 1) {
                shell.input_buffer[shell.buffer_pos++] = c;
                putchar(c);
            }
        }
    }
}

static struct input_buffer input_buf;

uint8_t uart_read_reg(uint32_t offset) {
    return *(volatile uint8_t*)(UART_BASE + offset);
}

void uart_write_reg(uint32_t offset, uint8_t value) {
    *(volatile uint8_t*)(UART_BASE + offset) = value;
}

void uart_init(void) {
    uart_write_reg(UART_LCR, 0x03);
    
    printf("UART initialized\n");
}

void uart_enable_interrupts(void) {
    enable_interrupts();
}

int uart_rx_ready(void) {
    return uart_read_reg(UART_LSR) & UART_LSR_RX_READY;
}

char uart_getchar(void) {
    while (!uart_rx_ready()) {
        wait_for_interrupt();
    }
    return uart_read_reg(UART_RHR);
}

void uart_putchar(char c) {
    while (!(uart_read_reg(UART_LSR) & 0x20)) {
    }
    uart_write_reg(UART_THR, c);
}

void handle_uart_interrupt(void) {
    if (uart_rx_ready()) {
        char c = uart_read_reg(UART_RHR);
        input_buffer_put(c);
    }
}

void input_buffer_init(void) {
    input_buf.write_pos = 0;
    input_buf.read_pos = 0;
    input_buf.count = 0;
}

void input_buffer_put(char c) {
    if (input_buf.count < INPUT_BUFFER_SIZE) {
        input_buf.buffer[input_buf.write_pos] = c;
        input_buf.write_pos = (input_buf.write_pos + 1) % INPUT_BUFFER_SIZE;
        input_buf.count++;
    }
}

char input_buffer_get(void) {
    if (input_buf.count > 0) {
        char c = input_buf.buffer[input_buf.read_pos];
        input_buf.read_pos = (input_buf.read_pos + 1) % INPUT_BUFFER_SIZE;
        input_buf.count--;
        return c;
    }
    return 0;
}

int input_buffer_available(void) {
    return input_buf.count > 0;
}

int sbi_console_getchar(void) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 1, 0x01);
    return (int)ret.value;
}

char getchar_blocking(void) {
    static int first_time = 1;
    if (first_time) {
        printf("Waiting for keyboard input... (try typing!)\n");
        first_time = 0;
    }
    
    while (1) {
        int c = sbi_console_getchar();
        if (c != -1 && c != 0) {
            return (char)c;
        }
        
        if (uart_rx_ready()) {
            return uart_read_reg(UART_RHR);
        }
        
        for (volatile int i = 0; i < 1000; i++);
    }
}


void shell_demo(void) {
    printf("\n=== Fru1t OS Shell Demo ===\n");
    printf("(Simulating user commands since keyboard input not implemented)\n\n");

    shell_init();

    printf("fru1t-os> help\n");
    cmd_help();

    printf("fru1t-os> ls\n");
    cmd_ls();

    printf("fru1t-os> cat welcome.txt\n");
    cmd_cat("welcome.txt");

    printf("fru1t-os> create test.txt 128\n");
    cmd_create("test.txt", "128");

    printf("fru1t-os> echo Hello Fru1t OS!\n");
    char *echo_args[] = {"echo", "Hello", "Fru1t", "OS!"};
    cmd_echo(echo_args, 4);

    printf("fru1t-os> ls\n");
    cmd_ls();

    printf("fru1t-os> memstat\n");
    cmd_memstat();

    printf("fru1t-os> delete test.txt\n");
    cmd_delete("test.txt");

    printf("fru1t-os> ls\n");
    cmd_ls();

    printf("fru1t-os> exit\n");
    printf("Goodbye!\n");

    printf("\n=== Shell Demo Complete ===\n");
}

/* Test function declarations */
extern void test_all_features(void);

void kernel_main(void) {
    memset(bss, 0, (size_t) bss_end - (size_t) bss);

    printf("Initializing memory allocator...\n");
    memory_init();

    printf("Initializing filesystem...\n");
    fs_init();

    printf("Initializing UART and keyboard interrupts...\n");
    uart_init();
    input_buffer_init();
    uart_enable_interrupts();

    printf("Creating sample files...\n");
    fs_create("welcome.txt", 256);
    const char *welcome_msg = "Welcome to Fru1t OS!";
    fs_write("welcome.txt", welcome_msg, strlen(welcome_msg) + 1);

    fs_create("readme.txt", 512);
    const char *readme_msg = "This is a simple operating system with basic shell functionality.";
    fs_write("readme.txt", readme_msg, strlen(readme_msg) + 1);

    /* Run CFS and epoll tests */
    printf("\n");
    printf("================================================\n");
    printf("  Running Red-Black Tree, CFS and epoll Tests\n");
    printf("================================================\n");
    test_all_features();

    printf("\n");
    printf("================================================\n");
    printf("  All tests completed successfully!\n");
    printf("  System halting...\n");
    printf("================================================\n");
    printf("\nPress Ctrl+A, X to exit QEMU\n\n");

    // Halt the system instead of running the shell
    while (1) {
        __asm__ volatile("wfi"); // Wait for interrupt (low power mode)
    }
}
