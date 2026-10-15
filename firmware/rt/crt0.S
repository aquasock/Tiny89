/* SPDX-License-Identifier: MIT */
    .section .text.start, "ax"
    .globl _start
_start:
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop
    la sp, __stack_top
    la t0, trap_entry
    csrw mtvec, t0
    li t0, 3
    csrs 0x7ca, t0
    fence.i

    la t1, __bss_start
    la t2, __bss_end
1:  bgeu t1, t2, 2f
    sw zero, 0(t1)
    addi t1, t1, 4
    j 1b
2:  call __libc_init_array
    li a0, 0
    li a1, 0
    li a2, 0
    call main
    call exit
3:  j 3b

    .text
    .align 2
trap_entry:
    la sp, __stack_top
    csrr a0, mcause
    csrr a1, mepc
    csrr a2, mtval
    call t89_rt_trap
4:  j 4b

    .globl t89_rt_reboot
    .type t89_rt_reboot, @function
t89_rt_reboot:
    li t0, 6
    csrw 0x7cc, t0
    li t0, 3
    csrc 0x7ca, t0
    fence.i
    li t0, 0x80000000
    jr t0
    .size t89_rt_reboot, . - t89_rt_reboot

    .section .note.GNU-stack, "", @progbits
