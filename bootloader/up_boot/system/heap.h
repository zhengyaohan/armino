#ifndef _HEAP_H_
#define _HEAP_H_

#include "BK_System.h"

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE	( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE		( ( size_t ) 8 )

#if (SOC_BK7271 == CFG_SOC_NAME)
#define configTOTAL_HEAP_SIZE       (64*1024)
#else
#define configTOTAL_HEAP_SIZE       (128*1024)
#endif
#define portBYTE_ALIGNMENT_MASK     (0x0007)
#define portBYTE_ALIGNMENT			8

#define mtCOVERAGE_TEST_MARKER() 

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< The next free block in the list. */
	size_t xBlockSize;						/*<< The size of the free block. */
} BlockLink_t;

extern void heap_init(void);

#endif
// eof

