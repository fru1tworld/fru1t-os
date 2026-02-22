.text
.global sbi_call
.global switch_to_user
.global read_csr_satp
.global write_csr_satp
.global switch_context
.global enable_interrupts
.global wait_for_interrupt

# SBI call function
# struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid)
sbi_call:
    # Arguments already in a0-a7 registers
    ecall
    # Return: a0 = error, a1 = value
    # Pack into 64-bit: (a1 << 32) | a0
    # But we're on 32-bit, so just return a0 for now
    ret

# Switch to user mode
# void switch_to_user(uint32_t pc, uint32_t sp, uint32_t satp)
switch_to_user:
    # a0 = pc, a1 = sp, a2 = satp
    csrw satp, a2
    sfence.vma
    
    # Set sstatus for user mode
    li t0, (1 << 8)        # SPP = 0 (user mode)
    csrc sstatus, t0
    li t0, (1 << 5)        # SPIE = 1 (enable interrupts after sret)
    csrs sstatus, t0
    
    # Set sepc and stack pointer
    csrw sepc, a0
    mv sp, a1
    
    sret

# Read SATP CSR
# uint32_t read_csr_satp(void)
read_csr_satp:
    csrr a0, satp
    ret

# Write SATP CSR  
# void write_csr_satp(uint32_t value)
write_csr_satp:
    csrw satp, a0
    sfence.vma
    ret

# Read SCAUSE CSR
# uint32_t read_csr_scause(void)
.global read_csr_scause
read_csr_scause:
    csrr a0, scause
    ret

# Read STVAL CSR
# uint32_t read_csr_stval(void)
.global read_csr_stval
read_csr_stval:
    csrr a0, stval
    ret

# Read SEPC CSR
# uint32_t read_csr_sepc(void)
.global read_csr_sepc
read_csr_sepc:
    csrr a0, sepc
    ret

# Write SEPC CSR
# void write_csr_sepc(uint32_t value)
.global write_csr_sepc
write_csr_sepc:
    csrw sepc, a0
    ret

# Context switch function
# void switch_context(uint32_t **old_sp, uint32_t *new_sp)
switch_context:
    # Save current context
    addi sp, sp, -64
    sw ra, 0(sp)
    sw s0, 4(sp)
    sw s1, 8(sp)
    sw s2, 12(sp)
    sw s3, 16(sp)
    sw s4, 20(sp)
    sw s5, 24(sp)
    sw s6, 28(sp)
    sw s7, 32(sp)
    sw s8, 36(sp)
    sw s9, 40(sp)
    sw s10, 44(sp)
    sw s11, 48(sp)
    
    # Save current stack pointer
    sw sp, 0(a0)
    
    # Load new stack pointer
    mv sp, a1
    
    # Restore new context
    lw s11, 48(sp)
    lw s10, 44(sp)
    lw s9, 40(sp)
    lw s8, 36(sp)
    lw s7, 32(sp)
    lw s6, 28(sp)
    lw s5, 24(sp)
    lw s4, 20(sp)
    lw s3, 16(sp)
    lw s2, 12(sp)
    lw s1, 8(sp)
    lw s0, 4(sp)
    lw ra, 0(sp)
    addi sp, sp, 64
    
    ret

# Enable interrupts
# void enable_interrupts(void)
enable_interrupts:
    # Read SIE register
    csrr t0, sie
    # Set SEIE (external interrupt enable)
    ori t0, t0, 0x200
    csrw sie, t0
    # Enable interrupts in sstatus
    csrsi sstatus, 0x2
    ret

# Wait for interrupt
# void wait_for_interrupt(void)  
wait_for_interrupt:
    wfi
    ret

# Kernel entry function for trap handling
# void kernel_entry(void)
.global kernel_entry
kernel_entry:
    csrw sscratch, sp
    addi sp, sp, -4 * 31
    sw ra,  4 * 0(sp)
    sw gp,  4 * 1(sp)
    sw tp,  4 * 2(sp)
    sw t0,  4 * 3(sp)
    sw t1,  4 * 4(sp)
    sw t2,  4 * 5(sp)
    sw t3,  4 * 6(sp)
    sw t4,  4 * 7(sp)
    sw t5,  4 * 8(sp)
    sw t6,  4 * 9(sp)
    sw a0,  4 * 10(sp)
    sw a1,  4 * 11(sp)
    sw a2,  4 * 12(sp)
    sw a3,  4 * 13(sp)
    sw a4,  4 * 14(sp)
    sw a5,  4 * 15(sp)
    sw a6,  4 * 16(sp)
    sw a7,  4 * 17(sp)
    sw s0,  4 * 18(sp)
    sw s1,  4 * 19(sp)
    sw s2,  4 * 20(sp)
    sw s3,  4 * 21(sp)
    sw s4,  4 * 22(sp)
    sw s5,  4 * 23(sp)
    sw s6,  4 * 24(sp)
    sw s7,  4 * 25(sp)
    sw s8,  4 * 26(sp)
    sw s9,  4 * 27(sp)
    sw s10, 4 * 28(sp)
    sw s11, 4 * 29(sp)
    csrr t0, sscratch
    sw t0, 4 * 30(sp)

    mv a0, sp
    call handle_trap

    lw ra,  4 * 0(sp)
    lw gp,  4 * 1(sp)
    lw tp,  4 * 2(sp)
    lw t0,  4 * 3(sp)
    lw t1,  4 * 4(sp)
    lw t2,  4 * 5(sp)
    lw t3,  4 * 6(sp)
    lw t4,  4 * 7(sp)
    lw t5,  4 * 8(sp)
    lw t6,  4 * 9(sp)
    lw a0,  4 * 10(sp)
    lw a1,  4 * 11(sp)
    lw a2,  4 * 12(sp)
    lw a3,  4 * 13(sp)
    lw a4,  4 * 14(sp)
    lw a5,  4 * 15(sp)
    lw a6,  4 * 16(sp)
    lw a7,  4 * 17(sp)
    lw s0,  4 * 18(sp)
    lw s1,  4 * 19(sp)
    lw s2,  4 * 20(sp)
    lw s3,  4 * 21(sp)
    lw s4,  4 * 22(sp)
    lw s5,  4 * 23(sp)
    lw s6,  4 * 24(sp)
    lw s7,  4 * 25(sp)
    lw s8,  4 * 26(sp)
    lw s9,  4 * 27(sp)
    lw s10, 4 * 28(sp)
    lw s11, 4 * 29(sp)
    lw sp,  4 * 30(sp)

    sret

# Boot function for kernel startup
# void boot(void)
.global boot
.section .text.boot
boot:
    mv tp, zero
    mv t0, zero
    mv t1, zero
    mv t2, zero
    mv t3, zero
    mv t4, zero
    mv t5, zero
    mv t6, zero
    mv ra, zero
    mv s0, zero
    mv s1, zero
    mv s2, zero
    mv s3, zero
    mv s4, zero
    mv s5, zero
    mv s6, zero
    mv s7, zero
    mv s8, zero
    mv s9, zero
    mv s10, zero
    mv s11, zero

    la t0, bss
    la t1, bss_end
1:  
    bge t0, t1, 2f
    sw zero, 0(t0)
    addi t0, t0, 4
    j 1b
2:
    la sp, __stack_top    
    call kernel_main     
3:  
    wfi              
    j 3b