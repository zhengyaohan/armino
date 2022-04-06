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
    EXPORT coef0
    EXPORT coef1
    EXPORT coef2
    EXPORT coef3    
    EXPORT iv_table_a
    EXPORT key_table_a
    EXPORT boot_version_a

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

separator1 DCD    0x10aabbee
coef0 DCD    0x510fb093
coef1 DCD    0xa3cbeadc
coef2 DCD    0x5993a17e
coef3 DCD    0xc7adeb03
separator2 DCD    0x32bbbbee
iv_table_a DCB "0123456789ABCDEF\0"
key_table_a DCB "0123456789ABCDEF0123456789ABCDEF\0"
wzl_padding DCB "ff"
separator3 DCD    0x0eccbbee
boot_version_a DCB "BK7231S_1.0.5\0"
wzh_padding DCB "FF"

;boot_breakpoint:
    ; If set to 0 by host before getting out of reset, the cpu
    ; will loop and light the leds indefinitely
;    .word   1
boot_breakpoint DCD    1
	END
