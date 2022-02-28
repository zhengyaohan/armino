/*
 * Copyright (c) 2021-2022 Beken Corporation
 * All rights reserved.
 *
 */

#include "core_v5.h"

.global  arch_disable_irq
.global  arch_enable_irq
.global  arch_int_lock
.global  arch_int_unlock
.global  arch_int_restore
.global  arch_fence
.global  arch_atomic_clear
.global  arch_atomic_spin_set

.section .text

/*
 * void arch_disable_irq(void);
 */
arch_disable_irq:
    li      t0, (MSTATUS_MPIE | MSTATUS_MIE) // mpie | mie
    csrrc   zero, mstatus, t0
    ret

/*
 * void arch_enable_irq(void);
 */
arch_enable_irq:
    csrsi  mstatus, MSTATUS_MIE
    ret

/*
 * u32 arch_int_lock(void);
 */
arch_int_lock:
    csrr    a0, mstatus           // return value
	csrci	mstatus, MSTATUS_MIE
    ret

/*
 * u32 arch_int_unlock(void);
 */
arch_int_unlock:
    csrr    a0, mstatus           // return value
	csrsi  mstatus, MSTATUS_MIE
    ret

/*
 * void arch_int_restore(u32 int_flag);
 */
arch_int_restore:
    csrw mstatus, a0
    ret

/*
 * void arch_fence(void);
 */
arch_fence:
	fence iorw, iorw
	ret

/*
 * void arch_atomic_clear(u32 * lock_addr);
 */
arch_atomic_clear:
	amoswap.w.rl x0, x0, (a0)
	ret

/*
 * void arch_atomic_spin_set(u32 * lock_addr);
 */
arch_atomic_spin_set:
	addi t0, x0, 1
swap_again: 
	amoswap.w.aq t0, t0, (a0)
	bnez t0, swap_again
	ret

