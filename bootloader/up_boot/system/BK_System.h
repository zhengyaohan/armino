/*************************************************************
 * @file        BK_System.h
 * @brief       Header file of BK_System.c
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 *
 * 2018-02-03   Murphy            add user memory function
 */

#ifndef  __BK_SYSTEM_H__

#define  __BK_SYSTEM_H__


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


#include "BK_config.h"

#define ASSERT(EXPR)                                                           \
if (!(EXPR))                                                                   \
{                                                                              \
    bk_printf("(%s) has assert failed at %s.\n", #EXPR, __FUNCTION__);            \
    while (1);                                                                 \
}


#if (MCU_CLK == MCU_CLK_26MHz)
#define tick_cnt_us         3
#define tick_cnt_ms         2700
#elif (MCU_CLK == MCU_CLK_120MHz)
#define tick_cnt_us         28
#define tick_cnt_ms         29500
#elif (MCU_CLK == MCU_CLK_180MHz)
#define tick_cnt_us         9
#define tick_cnt_ms         9000
#endif

/***************3.OTA type config***********************/
#define OTA_BK                                  1
#define OTA_ALI                                 2
#define OTA_RTT                                 3
#define CFG_OTA_TYPE                            OTA_RTT

/**************ota addr config**********************/
#if (CFG_OTA_TYPE == OTA_BK)
#define OS_CRC_BIN_START_ADDR 0x11000
#define OS_EX_ADDR            0x10000
#define OS1_HD_ADDR           0x1EF000
#define OTA_TWO_IMAGE_SWITCH_RUN       0
#elif (CFG_OTA_TYPE == OTA_ALI)
#define OS_EX_ADDR           0x10000
#else
#define OS_EX_ADDR            0x10000
#endif

/***************uart debug config*********************/
#define DEBUG_PORT              DEBUG_PORT_UART2
#define PRINT_PORT              DEBUG_PORT


/***********************************************/

#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))


#define NUMBER_ROUND_UP(a,b)        ((a) / (b) + (((a) % (b)) ? 1 : 0))
#define NUMBER_ROUND_DOWN(a,b)      ((a) / (b))


    typedef unsigned char       BYTE;
    typedef signed   char       int8;
    typedef signed   short      int16;
    typedef signed   long       int32;
    typedef signed   long long  int64;
    typedef unsigned char       uint8;
    typedef unsigned short      uint16;
    typedef unsigned long       uint32;
    typedef unsigned long long  uint64;
    typedef float               fp32;
    typedef double              fp64;

    typedef signed   char       s8;
    typedef signed   short      s16;
    typedef signed   long       s32;
    typedef unsigned char       uint8_t;
    typedef unsigned short      u16;
    typedef unsigned long       u32;
	typedef unsigned long       uint32;

    typedef unsigned char  		UINT8;      /* Unsigned  8 bit quantity        */
    typedef signed   char  		INT8;       /* Signed    8 bit quantity        */
    typedef unsigned short 		UINT16;     /* Unsigned 16 bit quantity        */
    typedef signed   short 		INT16;      /* Signed   16 bit quantity        */
    typedef unsigned int   		UINT32;     /* Unsigned 32 bit quantity        */
    typedef signed   int   		INT32;      /* Signed   32 bit quantity        */
    typedef unsigned long long  UINT64;		/* Unsigned 32 bit quantity        */
    typedef signed   long long  INT64;		/* Signed   32 bit quantity        */
    typedef float         		FP32;		/* Single precision floating point */
    typedef double         		FP64;		/* Double precision floating point */
    typedef unsigned int        size_t;
    typedef unsigned char       BOOLEAN;

    /* These types MUST be 16-bit or 32-bit */
    typedef int				INT;
    typedef unsigned int	UINT;

    /* This type MUST be 8-bit */
    typedef unsigned char	BYTE;

    /* These types MUST be 16-bit */
    typedef short			SHORT;
    typedef unsigned short	WORD;
    typedef unsigned short	WCHAR;

    /* These types MUST be 32-bit */
    typedef long			LONG;
    typedef unsigned long	DWORD;

    /* This type MUST be 64-bit (Remove this for ANSI C (C89) compatibility) */
    typedef unsigned long long QWORD;
    typedef enum
    {
        FALSE = 0,
        TRUE = !FALSE
    } bool, BOOL;

    typedef enum
    {
        OK = 0,
        ERROR = -1
    } STATUS;

    typedef enum
    {
        NO = 0,
        YES = 1
    } ASK;

#ifndef min_def
#define min_def
#define min( a, b ) ( ( a ) < ( b ) ) ? ( a ) : ( b )
#endif

#ifndef max_def
#define max_def
#define max( a, b ) ( ( a ) > ( b ) ) ? ( a ) : ( b )
#endif


typedef struct
{
    UINT32 world_flag;
    UINT32 vectors[8];
    UINT32 pad[3];
} world_struct;

typedef enum
{
    BK_OTA_RTT_TYPE,
    BK_OTA_ALI_TYPE,
    BK_OTA_ALI_AB,
    BK_OTA_FREERTOS_TYPE,
}OTA_TYPE;

typedef struct boot_param
{
    u32 magic;
    u32 upboot_addr;
    u32 os_addr;
    u32 ota_addr;
    OTA_TYPE ota_type;
    union
    {
        struct
        {
            u32 bkup_addr;
            u32 bkup_len;
            u32 ex_addr;
            u32 os_addr;
            u32 hd_addr;
            u32 crc;
            u32 status;
            u32 bk;
        }freertos_ota;
        
        struct
        {
            u32 dst_adr;
            u32 src_adr;
            u32 siz;
            u16 crc;
        }ali_ota;
    }ota;
    
}  BOOT_PARAM;


    extern void DelayNops(volatile unsigned long nops);

    extern void DelayUS(volatile unsigned long timesUS);
    extern void DelayMS(volatile unsigned long timesMS);

    extern void *malloc(unsigned int size);
    extern void *realloc(void *mem_address, unsigned int newsize);
    extern void *calloc(unsigned int n, unsigned int size);
    extern void free(void *ptr);
    extern void dump_hex(const unsigned char *ptr, int buflen);
    extern void bk_printf(const char *fmt, ...);

#define rt_malloc(size)                     malloc(size)
#define rt_realloc(mem_address, newsize)    realloc(mem_address, newsize)
#define rt_calloc(n, size)                  calloc(n, size)
#define rt_free(ptr)                        free(ptr)


#define REG_READ(addr)          *((volatile u32 *)(addr))
#define REG_WRITE(addr, _data) 	(*((volatile u32 *)(addr)) = (_data))

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __BK_SYSTEM_H__ */
