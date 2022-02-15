/*
 * Copyright (c) 2012-2017 Andes Technology Corporation
 * All rights reserved.
 *
 */
#include "core_v5.h"

	.section .nds_vector, "ax"

.option push
.option norelax
	.global reset_vector

reset_vector:
	/* Decide whether this is an NMI or cold reset */
//Remove the two line below to fix reboot issue. TO BE IMPROVED.
#	csrr t0, mcause
#	bnez t0, nmi_handler
	.global _start
	.type _start,@function

_start:
#ifndef CACHE_DISABLE
   /* enable icache   */
    csrr	a5,mcache_ctl
     ori	a5,a5,1
    csrw	mcache_ctl,a5
   /* enable dcache */
    csrr	a5,mcache_ctl
     ori	a5,a5,2
    csrw	mcache_ctl,a5
#endif
	/* Initialize global pointer */
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop

	/* Initialize stack pointer */
	la t0, _stack
	mv sp, t0

#ifdef __nds_execit
	/* Initialize EXEC.IT table */
	la t0, _ITB_BASE_
	csrw uitb, t0
#endif

#ifdef __riscv_flen
	/* Enable FPU */
	li t0, MSTATUS_FS
	csrrs t0, mstatus, t0

	/* Initialize FCSR */
	fscsr zero
#endif

	/* Initial machine trap-vector Base. Use FreeRTOS trap function. */
    #ifdef INT_VECTOR_EN
	    la t0, __vectors
	#else
	    la t0, freertos_risc_v_trap_handler
    #endif
	csrw mtvec, t0

	/* Enable vectored external PLIC interrupt */
	#ifdef INT_VECTOR_EN
	    csrsi mmisc_ctl, 2
	#endif

#ifdef CFG_CACHE_ENABLE
	/* Enable I/D cache with HW prefetcher and D-cache write-around (threshold: 4 cache lines) */
	li t0, (0x3 << 13)
	csrc mcache_ctl, t0
	li t0, (1 << 13) | (1 << 10) | (1 << 9) | (0x3)
	csrs mcache_ctl, t0
#endif

	/* Do system low level setup. It must be a leaf function */
	call __platform_init

	/* System reset handler */
	call reset_handler

	/* Infinite loop, if returned accidently */
1:	j 1b

	.weak __platform_init
__platform_init:
	ret



#	.weak trap_handler
#trap_handler:
#1:	j 1b

	.weak nmi_handler
nmi_handler:
	csrr t0, mcause						/* For viewing in the debugger only. */
	csrr t1, mepc						/* For viewing in the debugger only */
	csrr t2, mstatus
	j trap_entry						/* Handled by trap_entry. */

.org  0x100
.word 0x32374B42
.word 0x00003635
.option pop
	.section .text

	.global trap_entry
	.align 2

trap_entry:
#ifdef __riscv_flen
	addi sp, sp, -32*REGBYTES - 16*FPREGBYTES
#else
	addi sp, sp, -32*REGBYTES
#endif

	/* Save caller registers */
	STORE x1, 0*REGBYTES(sp)
	STORE x4, 1*REGBYTES(sp)
	STORE x5, 2*REGBYTES(sp)
	STORE x6, 3*REGBYTES(sp)
	STORE x7, 4*REGBYTES(sp)
	STORE x10, 5*REGBYTES(sp)
	STORE x11, 6*REGBYTES(sp)
	STORE x12, 7*REGBYTES(sp)
	STORE x13, 8*REGBYTES(sp)
	STORE x14, 9*REGBYTES(sp)
	STORE x15, 10*REGBYTES(sp)
#ifndef __riscv_32e
	STORE x16, 11*REGBYTES(sp)
	STORE x17, 12*REGBYTES(sp)
	STORE x28, 13*REGBYTES(sp)
	STORE x29, 14*REGBYTES(sp)
	STORE x30, 15*REGBYTES(sp)
	STORE x31, 16*REGBYTES(sp)
#endif

	/* Save mstatus and mepc */
	csrr t0, mepc
	csrr t1, mstatus

	STORE t0, 17*REGBYTES(sp)
	STORE t1, 18*REGBYTES(sp)

#ifdef __riscv_flen
	/* Read FPU status fcsr register */
	frcsr x5

	/* Save fp caller registers */
	FPSTORE f0, (20*REGBYTES + 0*FPREGBYTES)(sp)
	FPSTORE f1, (20*REGBYTES + 1*FPREGBYTES)(sp)
	FPSTORE f2, (20*REGBYTES + 2*FPREGBYTES)(sp)
	FPSTORE f3, (20*REGBYTES + 3*FPREGBYTES)(sp)
	FPSTORE f4, (20*REGBYTES + 4*FPREGBYTES)(sp)
	FPSTORE f5, (20*REGBYTES + 5*FPREGBYTES)(sp)
	FPSTORE f6, (20*REGBYTES + 6*FPREGBYTES)(sp)
	FPSTORE f7, (20*REGBYTES + 7*FPREGBYTES)(sp)
	FPSTORE f10, (20*REGBYTES + 8*FPREGBYTES)(sp)
	FPSTORE f11, (20*REGBYTES + 9*FPREGBYTES)(sp)
	FPSTORE f12, (20*REGBYTES + 10*FPREGBYTES)(sp)
	FPSTORE f13, (20*REGBYTES + 11*FPREGBYTES)(sp)
	FPSTORE f14, (20*REGBYTES + 12*FPREGBYTES)(sp)
	FPSTORE f15, (20*REGBYTES + 13*FPREGBYTES)(sp)
	FPSTORE f16, (20*REGBYTES + 14*FPREGBYTES)(sp)
	FPSTORE f17, (20*REGBYTES + 15*FPREGBYTES)(sp)
	FPSTORE f28, (20*REGBYTES + 16*FPREGBYTES)(sp)
	FPSTORE f29, (20*REGBYTES + 17*FPREGBYTES)(sp)
	FPSTORE f30, (20*REGBYTES + 18*FPREGBYTES)(sp)
	FPSTORE f31, (20*REGBYTES + 19*FPREGBYTES)(sp)

	/* Save fcsr register */
	sw x5, (20*REGBYTES + 20*FPREGBYTES)(sp)
#endif

	/* Call trap handler */
	csrr a0, mcause
	mv a1, sp
	call trap_handler

#ifdef __riscv_flen
	/* Restore fcsr register */
	lw x5, (20*REGBYTES + 20*FPREGBYTES)(sp)

	/* Restore fp caller registers */
	FPLOAD f0, (20*REGBYTES + 0*FPREGBYTES)(sp)
	FPLOAD f1, (20*REGBYTES + 1*FPREGBYTES)(sp)
	FPLOAD f2, (20*REGBYTES + 2*FPREGBYTES)(sp)
	FPLOAD f3, (20*REGBYTES + 3*FPREGBYTES)(sp)
	FPLOAD f4, (20*REGBYTES + 4*FPREGBYTES)(sp)
	FPLOAD f5, (20*REGBYTES + 5*FPREGBYTES)(sp)
	FPLOAD f6, (20*REGBYTES + 6*FPREGBYTES)(sp)
	FPLOAD f7, (20*REGBYTES + 7*FPREGBYTES)(sp)
	FPLOAD f10, (20*REGBYTES + 8*FPREGBYTES)(sp)
	FPLOAD f11, (20*REGBYTES + 9*FPREGBYTES)(sp)
	FPLOAD f12, (20*REGBYTES + 10*FPREGBYTES)(sp)
	FPLOAD f13, (20*REGBYTES + 11*FPREGBYTES)(sp)
	FPLOAD f14, (20*REGBYTES + 12*FPREGBYTES)(sp)
	FPLOAD f15, (20*REGBYTES + 13*FPREGBYTES)(sp)
	FPLOAD f16, (20*REGBYTES + 14*FPREGBYTES)(sp)
	FPLOAD f17, (20*REGBYTES + 15*FPREGBYTES)(sp)
	FPLOAD f28, (20*REGBYTES + 16*FPREGBYTES)(sp)
	FPLOAD f29, (20*REGBYTES + 17*FPREGBYTES)(sp)
	FPLOAD f30, (20*REGBYTES + 18*FPREGBYTES)(sp)
	FPLOAD f31, (20*REGBYTES + 19*FPREGBYTES)(sp)

	/* Write FPU status fcsr register */
	fscsr x5
#endif

	/* Disable interrupt */
	csrrci a5, mstatus, MSTATUS_MIE

	/* Restore mstatus and mepc */
	LOAD t0, 17*REGBYTES(sp)
	LOAD t1, 18*REGBYTES(sp)

	csrw mepc, t0
	csrw mstatus, t1

	/* Restore caller registers */
	LOAD x1, 0*REGBYTES(sp)
	LOAD x4, 1*REGBYTES(sp)
	LOAD x5, 2*REGBYTES(sp)
	LOAD x6, 3*REGBYTES(sp)
	LOAD x7, 4*REGBYTES(sp)
	LOAD x10, 5*REGBYTES(sp)
	LOAD x11, 6*REGBYTES(sp)
	LOAD x12, 7*REGBYTES(sp)
	LOAD x13, 8*REGBYTES(sp)
	LOAD x14, 9*REGBYTES(sp)
	LOAD x15, 10*REGBYTES(sp)
#ifndef __riscv_32e
	LOAD x16, 11*REGBYTES(sp)
	LOAD x17, 12*REGBYTES(sp)
	LOAD x28, 13*REGBYTES(sp)
	LOAD x29, 14*REGBYTES(sp)
	LOAD x30, 15*REGBYTES(sp)
	LOAD x31, 16*REGBYTES(sp)
#endif

#ifdef __riscv_flen
	addi sp, sp, 32*REGBYTES + 16*FPREGBYTES
#else
	addi sp, sp, 32*REGBYTES
#endif

	mret
#ifdef INT_VECTOR_EN
	.text

	.global default_irq_entry
	.align 2

default_irq_entry:
1:	j 1b

.macro INTERRUPT num
	.weak entry_irq\num
	.set entry_irq\num, default_irq_entry
	.long entry_irq\num
	.endm

	/* Vector table
	 * NOTE:
	 * The Vector Table base alignment requirement has to be :
	 * " 2^ceiling(log2(N)) x 4 " bytes,
	 * if the PLIC device supports N interrupt sources.
	 */
#define VECTOR_NUMINTRS         63
	.section .vector_table, "a"

	.global __vectors
//	.balign 4096
    .balign 128
__vectors:
	/* Trap vector */
	.long trap_entry

	/* PLIC interrupt vector */
	.altmacro
	.set irqno, 1
	.rept VECTOR_NUMINTRS
	INTERRUPT %irqno
	.set irqno, irqno+1
	.endr
#endif
