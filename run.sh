#!/bin/bash
set -xue

# clang 경로와 컴파일 옵션
CC=/opt/homebrew/opt/llvm/bin/clang  # Ubuntu 등 환경에 따라 경로 조정: CC=clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fno-stack-protector -ffreestanding -nostdlib"

# 커널 빌드 (Red-Black Tree, CFS, epoll, B-Tree, i-node 포함)
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
    kernel.c common.c asm_functions.s rbtree.c cfs.c fd.c epoll.c test_features.c btree.c inode.c test_btree_fs.c

# QEMU 실행
/opt/homebrew/bin/qemu-system-riscv32 \
    -machine virt \
    -bios default \
    -nographic \
    --no-reboot \
    -kernel kernel.elf
