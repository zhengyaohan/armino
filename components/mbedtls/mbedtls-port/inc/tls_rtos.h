#ifndef _TLS_RTOS_H_
#define _TLS_RTOS_H_



#include "bk_api_rtos.h"
#include "bk_api_mem.h"

#define os_calloc(nmemb,size)   ((size) && (nmemb) > (~( unsigned int) 0)/(size))?0:os_zalloc((nmemb)*(size))

#define TLS_EOK            0
#define TLS_ERROR          (-1)




#define TLS_NULL          0U

#endif

