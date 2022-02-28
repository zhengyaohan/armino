;/**
; ****************************************************************************************
; *
; * @file boot_vectors.s
; *
; * @brief ARM Exception Vectors table.
; *
; * Copyright (C) RivieraWaves 2011-2015
; *
; ****************************************************************************************
; */

	EXPORT vectors
	EXPORT boot_breakpoint	
	
	IMPORT boot_reset
    IMPORT undefined_instruction
    IMPORT software_interrupt
    IMPORT prefetch_abort
    IMPORT data_abort
    IMPORT not_used	
    IMPORT irq_handler
    IMPORT fiq_handler

	

    AREA SYS_BOOT, CODE, READONLY, ALIGN=4
    ENTRY


vectors
	b	boot_reset
	ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq
	ldr	pc, _fiq

_undefined_instruction DCD undefined_instruction
_software_interrupt DCD software_interrupt
_prefetch_abort DCD prefetch_abort
_data_abort DCD data_abort
_not_used DCD not_used	
_irq DCD irq_handler
_fiq DCD fiq_handler

    DCD 0xdeadbeef
    DCD 0xdeadbeef

boot_breakpoint DCD    1

	END
