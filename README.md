# Fru1t OS

A minimal operating system kernel written in C for the RISC-V 32-bit architecture, following the ["Writing an OS in 1000 lines"](https://github.com/nuta/operating-system-in-1000-lines) tutorial by nuta.

## About This Project

This is my implementation of the operating system tutorial from [operating-system-in-1000-lines](https://github.com/nuta/operating-system-in-1000-lines). I'm following the tutorial step by step to learn OS development fundamentals.

## Current Progress

- ✅ **Boot Process**: Custom bootloader entry point
- ✅ **Memory Management**: Basic physical memory allocator with page allocation
- ✅ **Trap Handling**: Exception and interrupt handling system
- ✅ **PANIC System**: Error handling with proper halt mechanism
- ✅ **Printf Support**: Formatted output for debugging
- ✅ **CPU Scheduling**: Round-robin scheduler with process management
- ✅ **Dynamic Memory Allocation**: Heap allocator with kmalloc/kfree
- ✅ **File System**: Simple in-memory filesystem with file operations
- ⏳ **System Calls**: (Planned)
- ⏳ **Virtual Memory**: (Planned)

## Prerequisites

### QEMU

Install QEMU with RISC-V support:

**macOS (Homebrew):**

```bash
brew install qemu
```

**Ubuntu/Debian:**

```bash
sudo apt-get update
sudo apt-get install qemu-system-misc
```

**Arch Linux:**

```bash
sudo pacman -S qemu-arch-extra
```

### Compiler

You need a RISC-V cross-compiler toolchain:

**macOS (Homebrew):**

```bash
brew install llvm
# The project uses Clang with RISC-V target support
```

**Ubuntu/Debian:**

```bash
sudo apt-get install clang
# or
sudo apt-get install gcc-riscv64-unknown-elf
```

**Building from source:**
If your distribution doesn't provide RISC-V toolchain, you can build it from source:

```bash
# Download and build riscv-gnu-toolchain
git clone https://github.com/riscv/riscv-gnu-toolchain
cd riscv-gnu-toolchain
./configure --prefix=/opt/riscv --with-arch=rv32ima --with-abi=ilp32
make
```

## Build and Run

### Quick Start

1. **Clone the repository:**

   ```bash
   git clone https://github.com/fru1tworld/fru1t-os.git
   cd fru1t-os
   ```

2. **Build and run:**
   ```bash
   chmod +x run.sh
   ./run.sh
   ```

### Manual Build

If you prefer to build manually:

```bash
# Set compiler (adjust path as needed)
CC=/opt/homebrew/opt/llvm/bin/clang  # macOS with Homebrew
# or
CC=clang  # if clang is in PATH
# or
CC=riscv32-unknown-elf-gcc  # if using riscv toolchain

# Compile
$CC -std=c11 -O2 -g3 -Wall -Wextra \
    --target=riscv32-unknown-elf \
    -fno-stack-protector -ffreestanding -nostdlib \
    -Wl,-Tkernel.ld -Wl,-Map=kernel.map \
    -o kernel.elf kernel.c common.c

# Run with QEMU
qemu-system-riscv32 \
    -machine virt \
    -bios default \
    -nographic \
    -serial mon:stdio \
    --no-reboot \
    -kernel kernel.elf
```

## Configuration

### Compiler Path

Edit `run.sh` to adjust the compiler path for your system:

```bash
# For macOS with Homebrew LLVM
CC=/opt/homebrew/opt/llvm/bin/clang

# For system-wide clang
CC=clang

# For RISC-V GCC toolchain
CC=riscv32-unknown-elf-gcc
```

### Memory Layout

The kernel uses the following memory layout (defined in `kernel.ld`):

- **Load Address**: `0x80200000`
- **Stack Size**: 64KB
- **Free RAM**: 64MB allocated for dynamic memory

## Project Structure

```
.
├── kernel.c        # Main kernel code
├── kernel.h        # Kernel headers and definitions
├── common.c        # Common utility functions (printf, memset)
├── common.h        # Common headers
├── kernel.ld       # Linker script
├── run.sh          # Build and run script
└── README.md       # This file
```

## Expected Output

When running successfully, you should see:

```
OpenSBI v1.5.1
...
[OpenSBI boot information]
...
Initializing memory allocator...
Memory allocator initialized: 1048576 bytes available
Initializing filesystem...
Filesystem initialized: 32 file slots available
Testing filesystem...

=== File System Test ===
Created file 'fru1tworld.txt' (512 bytes)
Created file 'fru1tworld_delete_test.txt' (256 bytes)

=== File System Listing ===
Files: 2/32
  fru1tworld.txt (512 bytes)
  fru1tworld_delete_test.txt (256 bytes)
Wrote 10 bytes to file 'fru1tworld.txt'
Wrote 58 bytes to file 'fru1tworld_delete_test.txt'
Read 512 bytes from file 'fru1tworld.txt'
Content of fru1tworld.txt: happy cat
Read 256 bytes from file 'fru1tworld_delete_test.txt'
Content of fru1tworld_delete_test.txt: This file will be deleted to test deletion functionality.
Deleted file 'fru1tworld_delete_test.txt'

=== File System Listing ===
Files: 1/32
  fru1tworld.txt (512 bytes)
Memory stats: 2 blocks, 1048040 bytes free, 512 bytes used
Kernel completed successfully!
```

The kernel demonstrates successful memory allocation, file system operations, and proper cleanup.

## Development

### Features Implemented

This OS includes the following key components:

#### 1. CPU Scheduling (Round-Robin)
```c
// Process states
#define PROC_UNUSED   0
#define PROC_READY    1
#define PROC_RUNNING  2
#define PROC_BLOCKED  3

struct process {
    int pid;
    int state;
    vaddr_t sp;
    uint32_t *page_table; 
    uint8_t stack[STACK_SIZE];
    struct trap_frame *trap_frame;
};

// Scheduler functions
void scheduler_init(void);
void schedule(void);
struct process *create_process(void (*entry_point)(void));
void yield(void);
void process_exit(void);
void context_switch(struct process *prev, struct process *next);
```

**Features:**
- Process creation and termination
- Round-robin scheduling algorithm
- Context switching with full register preservation
- Process state management (UNUSED, READY, RUNNING, BLOCKED)
- Support for up to 8 concurrent processes

#### 2. Dynamic Memory Allocation
```c
// Memory allocator structures
struct mem_block {
    int is_free;
    size_t size;
    struct mem_block *next;
};

// Allocator functions
void memory_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void print_memory_stats(void);
```

**Features:**
- 1MB heap space for dynamic allocation
- First-fit allocation algorithm
- Block splitting and coalescing
- Memory leak detection and statistics
- 8-byte aligned allocations

#### 3. Simple File System
```c
// File system structures
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

// File system functions
void fs_init(void);
int fs_create(const char *filename, size_t size);
int fs_write(const char *filename, const void *data, size_t size);
int fs_read(const char *filename, void *buffer, size_t size);
int fs_delete(const char *filename);
void fs_list(void);
int fs_exists(const char *filename);
```

**Features:**
- In-memory file system with 32 file slots
- File creation, reading, writing, and deletion
- Directory listing functionality
- File existence checking
- Integration with dynamic memory allocator
- Maximum file size of 1KB per file

### Development Branches

The project is organized into feature branches:

- `main`: Base kernel implementation
- `cpu-scheduling`: Round-robin process scheduler
- `memory-allocator`: Dynamic memory allocation system
- `simple-filesystem`: In-memory file system implementation

### Adding Features

When extending the OS:

1. **System Calls**: Extend `handle_syscall()` in `kernel.c`
2. **Memory Management**: Modify allocators or add virtual memory
3. **Device Drivers**: Add hardware abstraction layers
4. **Process Management**: Enhance scheduling algorithms
5. **File System**: Add persistent storage support

### Debugging

- Use `printf()` for debugging output
- Check `kernel.map` for symbol information
- Use QEMU's monitor commands (Ctrl+A, C to enter monitor)
- Add `-d in_asm,int,mmu` to QEMU for detailed debugging

### Exit QEMU

- **Ctrl+A, X**: Exit QEMU
- **Ctrl+A, C**: Enter QEMU monitor

## License

MIT License - see LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## References

- [Writing an OS in 1000 lines (Original Tutorial)](https://github.com/nuta/operating-system-in-1000-lines) - The main reference for this project
- [RISC-V Instruction Set Manual](https://riscv.org/technical/specifications/)
- [OpenSBI Documentation](https://github.com/riscv-software-src/opensbi)
- [QEMU RISC-V Documentation](https://www.qemu.org/docs/master/system/target-riscv.html)

## Learning Journey

This project represents my journey through the excellent tutorial by nuta. Each commit corresponds to different stages of the tutorial, making it easy to follow the progression from a simple bootloader to a full-featured microkernel.

If you're also following the tutorial, feel free to compare implementations or ask questions!
