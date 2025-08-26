# Fru1t OS

A minimal operating system kernel written in C for the RISC-V 32-bit architecture, featuring keyboard interrupt handling and interactive shell functionality.

## Features

- ✅ **Boot Process**: Custom bootloader entry point with OpenSBI integration
- ✅ **Memory Management**: Dynamic heap allocator with kmalloc/kfree (1MB heap)
- ✅ **Process Scheduling**: Round-robin scheduler supporting up to 8 processes
- ✅ **File System**: In-memory filesystem with 32 file slots (1KB max per file)
- ✅ **Trap Handling**: Complete exception and interrupt handling system
- ✅ **Keyboard Input**: UART-based keyboard input with interrupt and polling modes
- ✅ **Interactive Shell**: Command-line interface with 9 built-in commands
- ✅ **Input Buffer**: Circular buffer for keyboard input handling

## Shell Commands

The OS includes a fully functional shell with the following commands:

- `help` - Show available commands
- `ls` - List files in filesystem
- `cat <filename>` - Display file contents
- `create <filename> <size>` - Create new file
- `delete <filename>` - Delete file
- `echo [text]` - Print text to console
- `memstat` - Show memory allocation statistics
- `clear` - Clear screen
- `exit` - Exit shell

## Prerequisites

### QEMU
```bash
# macOS
brew install qemu

# Ubuntu/Debian
sudo apt-get install qemu-system-misc

# Arch Linux
sudo pacman -S qemu-arch-extra
```

### Compiler
```bash
# macOS (Recommended)
brew install llvm

# Ubuntu/Debian
sudo apt-get install clang
```

## Build and Run

### Quick Start
```bash
git clone <repository-url>
cd myos
chmod +x run.sh
./run.sh
```

### Manual Build
```bash
CC=/opt/homebrew/opt/llvm/bin/clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fno-stack-protector -ffreestanding -nostdlib"

$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf kernel.c common.c

qemu-system-riscv32 \
    -machine virt \
    -bios default \
    -nographic \
    --no-reboot \
    -kernel kernel.elf
```

## Expected Output

```
OpenSBI v1.5.1
...
Initializing memory allocator...
Memory allocator initialized: 1048576 bytes available
Initializing filesystem...
Filesystem initialized: 32 file slots available
Initializing UART and keyboard interrupts...
UART initialized
Creating sample files...
Created file 'welcome.txt' (256 bytes)
Wrote 21 bytes to file 'welcome.txt'
Created file 'readme.txt' (512 bytes)
Wrote 66 bytes to file 'readme.txt'
Starting shell demo (keyboard input will be simulated)...

=== Fru1t OS Shell Demo ===
(Simulating user commands since keyboard input not implemented)

fru1t-os> help

=== Fru1t OS Shell Commands ===
help          - Show this help message
ls            - List files in filesystem
cat <file>    - Display file contents
create <file> <size> - Create new file
delete <file> - Delete file
echo [args]   - Print arguments
memstat       - Show memory statistics
clear         - Clear screen
exit          - Exit shell

fru1t-os> ls

=== File System Listing ===
Files: 2/32
  welcome.txt (256 bytes)
  readme.txt (512 bytes)

fru1t-os> cat welcome.txt
Read 256 bytes from file 'welcome.txt'
Content of welcome.txt:
Welcome to Fru1t OS!

...
```

## Architecture

### Memory Layout
- **Load Address**: 0x80200000
- **Stack**: 64KB per process
- **Heap**: 1MB for dynamic allocation
- **Max Files**: 32 files, 1KB each

### Key Components

#### Process Management
- Round-robin scheduler with 10ms time slices
- Process states: UNUSED, READY, RUNNING, BLOCKED
- Full context switching with register preservation

#### Memory Allocator
- First-fit allocation algorithm
- Block splitting and coalescing
- 8-byte aligned allocations
- Memory leak detection

#### File System
- In-memory storage with dynamic allocation
- File operations: create, read, write, delete, list
- Filename limit: 64 characters
- File size limit: 1024 bytes

#### Input System
- UART-based keyboard input handling
- Interrupt and polling mode support
- Circular input buffer (256 bytes)
- Real-time command processing

## Project Structure

```
.
├── kernel.c        # Main kernel implementation
├── kernel.h        # Kernel headers and definitions
├── common.c        # Utility functions (printf, memset, etc.)
├── common.h        # Common headers
├── kernel.ld       # Linker script for memory layout
├── run.sh          # Build and run script
├── test_input.sh   # Keyboard input testing script
└── README.md       # This documentation
```

## Development

### Adding New Commands
1. Add command handler function in `kernel.c`
2. Update `shell_execute_command()` with new case
3. Add help text in `cmd_help()`

### Memory Management
- Use `kmalloc()` and `kfree()` for dynamic allocation
- Check `print_memory_stats()` for debugging leaks
- Heap size configurable via `HEAP_SIZE` define

### File System Extension
- Modify `MAX_FILES` and `MAX_FILESIZE` for different limits
- Add new file operations in filesystem section
- Files are stored in RAM and lost on reboot

## Controls

- **Exit QEMU**: Ctrl+A, X
- **QEMU Monitor**: Ctrl+A, C
- **Shell Navigation**: Standard terminal controls

## Known Issues

- Keyboard input may not work properly in some QEMU configurations
- Files are not persistent (RAM-only storage)
- Limited to 32-bit RISC-V architecture
- No virtual memory management

## License

MIT License - Feel free to use and modify.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Implement your changes
4. Test thoroughly with `./run.sh`
5. Submit a pull request

## References

- [RISC-V Instruction Set Manual](https://riscv.org/technical/specifications/)
- [OpenSBI Documentation](https://github.com/riscv-software-src/opensbi)
- [QEMU RISC-V Documentation](https://www.qemu.org/docs/master/system/target-riscv.html)