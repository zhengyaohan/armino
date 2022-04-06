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
    EXPORT undefined_instruction
    EXPORT prefetch_abort
    EXPORT data_abort
    EXPORT not_used
    EXPORT software_interrupt
    EXPORT irq_handler
    EXPORT fiq_handler
    EXPORT do_swi
    EXPORT do_irq
    EXPORT do_fiq
    EXPORT do_undefined_instruction   
    EXPORT __user_initial_stackheap
    
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
    IMPORT do_anomaly_print
    IMPORT do_software_interrupt
	IMPORT intc_irq
	IMPORT intc_fiq


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


WORLD_MAGIC_WORD_ADDR       EQU     0x00400000

OS_IRQ_ADDR     EQU     0x00400000
OS_FIQ_ADDR		EQU     0x00400004
OS_SWI_ADDR		EQU     0x00400008


SCTRL_BASE                  EQU         (0x00800000)
SCTRL_BLOCK_EN_MUX			EQU 		(SCTRL_BASE + 79 * 4)
SCTRL_BLOCK_EN_CFG          EQU         (SCTRL_BASE + 75 * 4)
BLOCK_EN_WORD_POSI          EQU         (20)
BLOCK_EN_WORD_MASK          EQU         (0xFFF)
BLOCK_EN_WORD_PWD           EQU         (0xA5C)
BLK_EN_XTAL2RF              EQU         (1 << 10)
BLK_EN_DPLL_480M            EQU         (1 << 05)
BLK_EN_26M_XTAL             EQU         (1 << 03)
BLK_EN_DCO                  EQU         (1 << 01)
SCTRL_BLOCK_VALUE_SET       EQU         ((BLOCK_EN_WORD_PWD << BLOCK_EN_WORD_POSI) \
        + BLK_EN_DPLL_480M + BLK_EN_26M_XTAL + BLK_EN_DCO)

SCTRL_CONTROL               EQU         (SCTRL_BASE + 02 * 4)
QSPI_IO_33                  EQU         (1 << 17)
MCLK_DIV_3                  EQU         (3)
MCLK_DIV_POSI               EQU         (4)
MCLK_FIELD_DPLL             EQU         (0x2)
MCLK_MUX_POSI               EQU         (0)
SCTRL_CONTROL_VALUE_SET     EQU         (QSPI_IO_33 + (MCLK_DIV_3 << MCLK_DIV_POSI) \
                                + (MCLK_FIELD_DPLL<<MCLK_MUX_POSI))

ARM_REG_SAVED_SVC_START     EQU         0x400020
ARM_REG_SAVED_SVC_LEN       EQU         0x40
ARM_REG_SAVED_SVC_END       EQU         (ARM_REG_SAVED_SVC_START + ARM_REG_SAVED_SVC_LEN)
ARM_REG_SAVED_IRQ_START     EQU         0x400064
ARM_REG_SAVED_IRQ_LEN       EQU         0x24
ARM_REG_SAVED_IRQ_END       EQU         (ARM_REG_SAVED_IRQ_START + ARM_REG_SAVED_IRQ_LEN)
ARM_REG_SAVED_FIQ_START     EQU         ARM_REG_SAVED_IRQ_END
ARM_REG_SAVED_FIQ_LEN       EQU         0x24
ARM_REG_SAVED_FIQ_END       EQU         (ARM_REG_SAVED_FIQ_START + ARM_REG_SAVED_FIQ_LEN)
ARM_REG_SAVED_ABT_START     EQU         ARM_REG_SAVED_FIQ_END
ARM_REG_SAVED_ABT_LEN       EQU         0x24
ARM_REG_SAVED_ABT_END       EQU         (ARM_REG_SAVED_ABT_START + ARM_REG_SAVED_ABT_LEN)
ARM_REG_SAVED_UND_START     EQU         ARM_REG_SAVED_ABT_END
ARM_REG_SAVED_UND_LEN       EQU         0x24
ARM_REG_SAVED_UND_END       EQU         (ARM_REG_SAVED_UND_START + ARM_REG_SAVED_UND_LEN)
ARM_REG_SAVED_SYS_START     EQU         ARM_REG_SAVED_UND_END
ARM_REG_SAVED_SYS_LEN       EQU         0x20
ARM_REG_SAVED_SYS_END       EQU         (ARM_REG_SAVED_SYS_START + ARM_REG_SAVED_SYS_LEN)



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

	GBLL __SOC_BK7271
__SOC_BK7271 SETL {FALSE}
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



    MACRO
    PUSH_ALL_ARM_REG
        BOOT_CHANGE_MODE  IRQ
        LDR R1, =ARM_REG_SAVED_IRQ_END           ;// get backup top.
        STMFD   R1!, {R8-R14}       ;// backup R8-R14
        MRS   R0, SPSR
        STMFD   R1!, {R0}           ;// backup SPSR
        MRS   R0, CPSR
        STMFD   R1!, {R0}           ;// backup CPSR

        BOOT_CHANGE_MODE  FIQ
        LDR R1, =ARM_REG_SAVED_FIQ_END           ;// get backup top.
        STMFD   R1!, {R8-R14}       ;// backup R8-R14
        MRS   R0, SPSR
        STMFD   R1!, {R0}           ;// backup SPSR
        MRS   R0, CPSR
        STMFD   R1!, {R0}           ;// backup CPSR


        BOOT_CHANGE_MODE  ABT
        LDR R1, =ARM_REG_SAVED_ABT_END           ;// get backup top.
        STMFD   R1!, {R8-R14}       ;// backup R8-R14
        MRS   R0, SPSR
        STMFD   R1!, {R0}           ;// backup SPSR
        MRS   R0, CPSR
        STMFD   R1!, {R0}           ;// backup CPSR


        BOOT_CHANGE_MODE  UND
        LDR R1, =ARM_REG_SAVED_UND_END           ;// get backup top.
        STMFD   R1!, {R8-R14}       ;// backup R8-R14
        MRS   R0, SPSR
        STMFD   R1!, {R0}           ;// backup SPSR
        MRS   R0, CPSR
        STMFD   R1!, {R0}           ;// backup CPSR


        BOOT_CHANGE_MODE  SYS
        LDR R1, =ARM_REG_SAVED_SYS_END           ;// get backup top.
        STMFD   R1!, {R8-R14}       ;// backup R8-R14
        MRS   R0, CPSR
        STMFD   R1!, {R0}           ;// backup CPSR


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
    MOV R10, R3                 ; backup R3 to R10.
    LDR R3, =ARM_REG_SAVED_SVC_END           ; get backup top.
    STMFD   R3!, {R4-R14}       ; backup R4-R14
    STMFD   R3!, {R10}          ; backup R3
    STMFD   R3!, {R0-R2}        ; backup R0-R2

    ; Disable IRQ and FIQ before starting anything
    ;IMPORT  CopyCode2Ram_boot
    MRS   R0, CPSR
    STMFD   R3!, {R0}           ; backup CPSR
    ORR   R0, R0, #0xC0
    MSR   CPSR_c, R0

    PUSH_ALL_ARM_REG
    BOOT_CHANGE_MODE SVC

   [	__SOC_BK7271={TRUE}
    ;Recover flash clock setting from bootrom
    LDR  R0, =0x0080301C
    LDR  R1, =0x0400000F
    STR  R1, [R0, #0]
    ]

	[	__SOC_BK7271={FALSE}
    ;clear block mux
    LDR  R0, =SCTRL_BLOCK_EN_MUX
    LDR  R1, =0x0
    STR  R1, [R0, #0]

    ; block en  BLK_BIT_26M_XTAL | BLK_BIT_DPLL_480M | BLK_BIT_DCO;
    LDR  R0, =SCTRL_BLOCK_EN_CFG
    LDR  R1, =SCTRL_BLOCK_VALUE_SET
    STR  R1, [R0, #0]
	
    ; mclk 120M
    LDR  R0, =SCTRL_CONTROL
    LDR  R1, =SCTRL_CONTROL_VALUE_SET
    STR  R1, [R0, #0]
    ]

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
    
    BL InitStack

    ;LDR R0, =CopyCode2Ram_boot
    ;BLX R0
    
    B boot_main

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

; /*FUNCTION:     _sysboot_zi_init*/
; /*DESCRIPTION:  Initialise Zero-Init Data Segment*/;
_sysboot_zi_init
        LDR     R0, =||Image$$RAM_BSS$$Base||
        LDR     R1, =||Image$$RAM_BSS$$ZI$$Length||
        
        ADD R3, R1, R0
        MOV R4, R0
        MOV R2, #0
_zi_loop
        CMP R4, R3
        STRLO R2, [R4], #4
        BLO _zi_loop
        BX LR



I_BIT           EQU     0x80 ; when I bit is set, IRQ is disabled
F_BIT           EQU     0x40 ; when F bit is set, FIQ is disabled

OFFSET_FIQ_STACK         EQU     0            
OFFSET_IRQ_STACK         EQU     0x3F0
OFFSET_SVC_STACK         EQU     0x43F0
OFFSET_UND_STACK         EQU     0x4400

InitStack
        MOV     R0, LR        
        BX      R0 

STACK_BASE      DCD      ||Image$$RAM_STACK_FIQ$$Base||



;/*
; * exception handlers
; */
	ALIGN
do_undefined_instruction
    LDMFD   SP!, {R0-R1}
	get_bad_stack
	bad_save_user_regs
	bl	do_anomaly_print

    ALIGN
software_interrupt
    ;;ADD LR, LR, #4
	STMFD  sp!,{r0-r1}
	LDR    R1, =0x400008
	LDR    R1, [R1]
	BX     R1

    ALIGN
irq_handler
	STMFD  sp!,{r0-r1}
	LDR    R1, =0x400000
	LDR    R1, [R1]
	BX     R1


    ALIGN
fiq_handler
	STMFD  sp!,{r0-r1}
	LDR    R1, =0x400004
	LDR    R1, [R1]
	BX     R1


    ALIGN
do_swi
    LDMFD   SP!, {R0-R1}
    STMDB SP!, {r0-r12,r14}
    BL do_software_interrupt
	LDMIA SP!, {r0-r12,r14}
    SUBS PC,R14,#0x04


    ALIGN
do_irq
    LDMFD   SP!, {R0-R1}
	STMDB SP!, {r0-r12,r14}
	BL intc_irq
	LDMIA SP!, {r0-r12,r14}
	SUBS PC,R14,#0x04

    ALIGN
do_fiq
    LDMFD   SP!, {R0-R1}
	STMDB SP!, {r0-r12,r14}
	BL intc_fiq
	LDMIA SP!, {r0-r12,r14}
	SUBS PC,R14,#0x04
	
	ALIGN
undefined_instruction
	STMFD  sp!,{r0-r1}
	LDR    R1, =0x40000c
	LDR    R1, [R1]
	BX     R1
    
	ALIGN
prefetch_abort
	STMFD  sp!,{r0-r1}
	LDR    R1, =0x400010
	LDR    R1, [R1]
	BX     R1

	ALIGN
data_abort
	STMFD  sp!,{r0-r1}
	LDR    R1, =0x400014
	LDR    R1, [R1]
	BX     R1

	ALIGN
not_used
	STMFD  sp!,{r0-r1}
	LDR    R1, =0x400018
	LDR    R1, [R1]
	BX     R1
	
    
__user_initial_stackheap    
        LDR     r0,=bottom_of_heap
        BX      lr ;MOV   pc,lr

__rt_div0
        B       __rt_div0


set_world_flag
    PUSH {r0, r1}
    LDR R0,=WORLD_MAGIC_WORD_ADDR
    MOV R1,#0
    STR R1,[R0]
    POP {r0, r1}
    BX lr

        AREA    Myheap, DATA, NOINIT, ALIGN=2
bottom_of_heap     SPACE   256  ;
        
	END
