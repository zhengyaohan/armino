#include <stdio.h>
#include "bk_log.h"

#include "bk_api_c2.h"

#define TAG "exam"

int c3_public_api(void)
{
	BK_LOGI(TAG, "Call %s\n", __FUNCTION__);
	return 0;
}
