#include "sdkconfig.h"
#include "multi_config.h"
#include <components/log.h>

#define TAG "multi_config"

int main(void)
{
	BK_LOGI(TAG, "Application name: %s\n", CONFIG_EXAMPLE_PRODUCT_NAME);
	multi_config_api();
	return BK_OK;
}
