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
	IMPORT boot_reset	


    EXPORT coef0
    EXPORT coef1
    EXPORT coef2
    EXPORT coef3
    
    EXPORT iv_table_a
    EXPORT key_table_a
	
;    .align  4
;    .global vectors, boot_breakpoint
;    .type   vectors, function


    AREA SYS_BOOT, CODE, READONLY, ALIGN=4
    ENTRY


vectors
	b	boot_reset

separator1 DCD    0x0
coef0 DCD    0x12345678
coef1 DCD    0x2faa55aa
coef2 DCD    0x3aee63dd
coef3 DCD    0x4feeaa00
separator2 DCD    0x0
iv_table_a DCB "0123456789ABCDEF\0"
key_table_a DCB "0123456789ABCDEF0123456789ABCDEF\0"
resv_padding0 DCB "0\0"
separator3 DCD    0x0

boot_breakpoint DCD    1

	END
