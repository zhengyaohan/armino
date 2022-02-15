;/**
; ****************************************************************************************
; *
; * @file boot_handlers.s
; *
; * @brief ARM Exception Vector handler functions.
; *
; * Copyright (C) RivieraWaves 2011-2015
; *
; ****************************************************************************************
; */

    PRESERVE8
    AREA SYS_BOOT, CODE, READONLY

	EXPORT boot_reset

    
    
	IMPORT ||Load$$RAM_DATA$$Base||  
	
	IMPORT ||Image$$RAM_DATA$$Base||               ;//ram_data_base
	IMPORT ||Image$$RAM_DATA$$Length||             ;//ram_data_length
	
	IMPORT ||Image$$RAM_BSS$$Base||                ;//bss_base
	IMPORT ||Image$$RAM_BSS$$Length||              ;//image_bss_length
	IMPORT ||Image$$RAM_BSS$$ZI$$Length||          ;//bss_length
	
	IMPORT ||Image$$RAM_STACK_UNUSED$$Base||       ;//stack_base_unused
	IMPORT ||Image$$RAM_STACK_UNUSED$$ZI$$Length|| ;//stack_len_unused	
	
	IMPORT ||Image$$RAM_STACK_IRQ$$Base||          ;//stack_base_irq
	IMPORT ||Image$$RAM_STACK_IRQ$$ZI$$Length||    ;//stack_len_irq	
	
	IMPORT ||Image$$RAM_STACK_SVC$$Base||          ;//stack_base_svc
	IMPORT ||Image$$RAM_STACK_SVC$$ZI$$Length||    ;//stack_len_svc	
	
	IMPORT ||Image$$RAM_STACK_FIQ$$Base||          ;//stack_base_fiq
	IMPORT ||Image$$RAM_STACK_FIQ$$ZI$$Length||    ;//stack_len_fiq
	
	IMPORT boot_main
    
	


;/* ========================================================================
; *                                Constants
; * ======================================================================== */
BOOT_MODE_MASK    EQU 0x1F
BOOT_MODE_USR      EQU 0x10
BOOT_MODE_FIQ      EQU 0x11
BOOT_MODE_IRQ      EQU 0x12
BOOT_MODE_SVC      EQU 0x13
BOOT_MODE_ABT      EQU 0x17
BOOT_MODE_UND      EQU 0x1B
BOOT_MODE_SYS      EQU 0x1F
BOOT_FIQ_IRQ_MASK  EQU 0xC0
BOOT_IRQ_MASK      EQU 0x80

BOOT_COLOR_UNUSED  EQU 0xAAAAAAAA      ; Pattern to fill UNUSED stack
BOOT_COLOR_SVC     EQU 0xBBBBBBBB      ; Pattern to fill SVC stack
BOOT_COLOR_IRQ     EQU 0xCCCCCCCC      ; Pattern to fill IRQ stack
BOOT_COLOR_FIQ     EQU 0xDDDDDDDD      ; Pattern to fill FIQ stack


;/*
; *************************************************************************
; * Interrupt handling
; *************************************************************************
; */
S_FRAME_SIZE EQU	72
S_OLD_R0 EQU	    68

S_PSR    EQU		64
S_PC     EQU		60
S_LR     EQU		56
S_SP     EQU		52

S_IP     EQU		48
S_FP     EQU		44
S_R10    EQU		40
S_R9     EQU		36
S_R8     EQU		32
S_R7     EQU		28
S_R6     EQU		24
S_R5     EQU		20
S_R4     EQU		16
S_R3     EQU		12
S_R2     EQU		8
S_R1     EQU		4
S_R0     EQU		0

MODE_SVC EQU       0x13

;/* ========================================================================
; *                                Macros
; * ======================================================================== */
	MACRO
	bad_save_user_regs
	;@ carve out a frame on current user stack
	sub	sp, sp, #S_FRAME_SIZE
	stmia	sp, {r0 - r12}	;@ Save user registers (now in svc mode) r0-r12

    ldr	r2, boot_stack_len_SVC
    
	;@ get values for "aborted" pc and cpsr (into parm regs)
	ldmia	r2, {r2 - r3}
	add	r0, sp, #S_FRAME_SIZE		    ;@ grab pointer to old stack
	add	r5, sp, #S_SP
	mov	r1, lr
	stmia	r5, {r0 - r3}	;@ save sp_SVC, lr_SVC, pc, cpsr
	mov	r0, sp		        ;@ save current stack into r0 (param register)
	MEND

	MACRO
    get_bad_stack
    ldr	r2, boot_stack_len_SVC
    
	str	lr, [r13]	    ;@ save caller lr in position 0 of saved stack
	mrs	lr, spsr	    ;@ get the spsr
	str	lr, [r13, #4]	;@ save spsr in position 1 of saved stack
	mov	r13, #MODE_SVC	;@ prepare SVC-Mode
	msr	spsr_cxsf, r13	;@ switch modes, make sure moves will execute
	mov	lr, pc		    ;@ capture return pc
	movs pc, lr		;@ jump to next instruction & switch modes.
	MEND
;/* ========================================================================
;/**
; * Macro for switching ARM mode
; */
    MACRO 
	BOOT_CHANGE_MODE $newmode
        MRS   R0, CPSR
        BIC   R0, R0, #BOOT_MODE_MASK
        ORR   R0, R0, #BOOT_MODE_$newmode
        MSR   CPSR_c, R0
    MEND

;/* ========================================================================
;/**
; * Macro for setting the stack
; */
    MACRO
$x  BOOT_SET_STACK $stackName
        LDR   R0, boot_stack_base_$stackName
        LDR   R2, boot_stack_len_$stackName
        ADD   R1, R0, R2
        MOV   SP, R1        ; Set stack pointer

        LDR   R2, =BOOT_COLOR_$stackName

90      CMP   R0, R1        ; End of stack?
        STRLT R2, [r0]      ; Colorize stack word
        ADDLT R0, R0, #4
        BLT   %B90          ; branch to previous local label

    MEND

;/* ========================================================================
; *                                Globals
; * ======================================================================== */

;/* ========================================================================
;/**
; * CP15 ITCM Control Reg settings
; */
;boot_Cp15ItcmReg:
;    .word 0x01000001
boot_Cp15ItcmReg DCD 0x01000001

;/* ========================================================================
;/**
; * RAM_BSS
; */
;ram_bss_base:
;    .word bss_base
ram_bss_base DCD ||Image$$RAM_BSS$$Base||

;ram_bss_length:
;    .word bss_length
ram_bss_length DCD ||Image$$RAM_BSS$$ZI$$Length||

;/* ========================================================================
;/**
; * Unused (ABT, UNDEFINED, SYSUSR) Mode
; */
;boot_stack_base_UNUSED:
;    .word stack_base_unused
boot_stack_base_UNUSED DCD ||Image$$RAM_STACK_UNUSED$$Base||

;boot_stack_len_UNUSED:
;    .word stack_len_unused
boot_stack_len_UNUSED DCD ||Image$$RAM_STACK_UNUSED$$ZI$$Length||

;/* ========================================================================
;/**
; * IRQ Mode
; */
;boot_stack_base_IRQ:
;    .word stack_base_irq
boot_stack_base_IRQ DCD ||Image$$RAM_STACK_IRQ$$Base||

;boot_stack_len_IRQ:
;    .word stack_len_irq
boot_stack_len_IRQ DCD ||Image$$RAM_STACK_IRQ$$ZI$$Length||


;/* ========================================================================
;/**
; * Supervisor Mode
; */
;boot_stack_base_SVC:
;    .word stack_base_svc
boot_stack_base_SVC DCD ||Image$$RAM_STACK_SVC$$Base||

;boot_stack_len_SVC:
;    .word stack_len_svc
boot_stack_len_SVC DCD ||Image$$RAM_STACK_SVC$$ZI$$Length||


;/* ========================================================================
;/**
; * FIQ Mode
; */
;boot_stack_base_FIQ:
;    .word stack_base_fiq
boot_stack_base_FIQ DCD ||Image$$RAM_STACK_FIQ$$Base||

;boot_stack_len_FIQ:
;    .word stack_len_fiq
boot_stack_len_FIQ DCD ||Image$$RAM_STACK_FIQ$$ZI$$Length||



;/* ========================================================================
;/**
; * Function to handle reset vector
; */
boot_reset
    ; Disable IRQ and FIQ before starting anything
    ;IMPORT  CopyCode2Ram_boot
    MRS   R0, CPSR
    ORR   R0, R0, #0xC0
    MSR   CPSR_c, R0

    ; Setup all stacks
    ; Note: Sys and Usr mode are not used
    BOOT_CHANGE_MODE SYS
    BOOT_SET_STACK   UNUSED
    
    BOOT_CHANGE_MODE ABT
    BOOT_SET_STACK   UNUSED
    
    BOOT_CHANGE_MODE UND
    BOOT_SET_STACK   UNUSED
    
    BOOT_CHANGE_MODE IRQ
    BOOT_SET_STACK   IRQ
    
    BOOT_CHANGE_MODE FIQ
    BOOT_SET_STACK   FIQ

    ; Clear FIQ banked registers while in FIQ mode
    MOV     R8,  #0
    MOV     R9,  #0
    MOV     R10, #0
    MOV     R11, #0
    MOV     R12, #0

    BOOT_CHANGE_MODE SVC
    BOOT_SET_STACK   SVC

    ; Stay in Supervisor Mode
    ;copy data from binary to ram
    BL _sysboot_copy_data_to_ram
    
    ; Init the BSS section
    LDR     R0, ram_bss_base
    LDR     R1, ram_bss_length
    MOV     R2, #0
    MOV     R3, #0
    MOV     R4, #0
    MOV     R5, #0
init_bss_loop
    SUBS    R1, R1, #16
    STMCSIA R0!, {R2, R3, R4, R5}
    BHI     init_bss_loop
    LSLS    R1, R1, #29
    STMCSIA R0!, {R4, R5}
    STRMI   R3, [R0]

    ; ==================
    ; Clear Registers
    MOV R0, #0
    MOV R1, #0
    MOV R2, #0
    MOV R3, #0
    MOV R4, #0
    MOV R5, #0
    MOV R6, #0
    MOV R7, #0
    MOV R8, #0
    MOV R9, #0
    MOV R10, #0
    MOV R11, #0
    MOV R12, #0
    
    B boot_main


InitStack
        MOV     R0, LR        
        BX      R0 ; MOV     PC, R0

STACK_BASE      DCD      ||Image$$RAM_STACK_FIQ$$Base||




; /*FUNCTION:     _sysboot_copy_data_to_ram*/
; /*DESCRIPTION:  copy main stack code from FLASH/ROM to SRAM*/
_sysboot_copy_data_to_ram
        LDR     R0, =||Load$$RAM_DATA$$Base||
        LDR     R1, =||Image$$RAM_DATA$$Base||
        LDR     R2, =||Image$$RAM_DATA$$Length||
		
        MOV     R3, R1
        ADD     R3, R3, R2
_rw_copy                                
        CMP R1, R3
        LDRLO   R4, [R0], #4
        STRLO   R4, [R1], #4
        BLO     _rw_copy
        BX LR
        
        
	END
