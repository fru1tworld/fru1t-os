#!/bin/bash
set -xue

# 빌드
CC=/opt/homebrew/opt/llvm/bin/clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fno-stack-protector -ffreestanding -nostdlib"

$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf kernel.c common.c

echo "Starting QEMU with keyboard input test..."
echo "After OS boots, you can try typing commands like 'help', 'ls', etc."
echo "Press Ctrl+A then X to exit QEMU"

# QEMU 실행 - 실제 키보드 입력 테스트
/opt/homebrew/bin/qemu-system-riscv32 \
    -machine virt \
    -bios default \
    -nographic \
    -serial mon:stdio \
    --no-reboot \
    -kernel kernel.elf