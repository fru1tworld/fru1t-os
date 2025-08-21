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
- 🚧 **Process Management**: (In progress)
- ⏳ **System Calls**: (Planned)
- ⏳ **File System**: (Planned)

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
alloc_pages test: paddr0=80201000
alloc_pages test: paddr1=80203000
PANIC: kernel.c:153: booted!
```

The kernel will halt after displaying the PANIC message, which indicates successful boot and memory allocation test.

## Development

### Following the Tutorial

This implementation follows the chapter progression from the original tutorial:

1. **Hello World**: Basic bootloader and printf functionality ✅
2. **Exception Handling**: Trap handlers and PANIC system ✅
3. **Memory Management**: Physical page allocator ✅
4. **Process Management**: User processes and context switching 🚧
5. **System Calls**: Process management syscalls ⏳
6. **Paging**: Virtual memory management ⏳
7. **File System**: Basic file operations ⏳
8. **Command Shell**: Interactive user interface ⏳

### Adding Features

When extending beyond the tutorial:

1. **System Calls**: Extend `handle_syscall()` in `kernel.c`
2. **Memory Management**: Modify `alloc_pages()` or add new allocators
3. **Device Drivers**: Add hardware abstraction in new source files
4. **Process Management**: Implement scheduling and process structures

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
