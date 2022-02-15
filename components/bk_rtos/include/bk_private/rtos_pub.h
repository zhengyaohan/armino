#pragma once

#include "bk_api_rtos.h"
#include "bk_api_uart.h"
#include "bk_log.h"

#define RTOS_TAG "rtos"

#define RTOS_LOGI(...) BK_LOGI(RTOS_TAG, ##__VA_ARGS__)
#define RTOS_LOGW(...) BK_LOGW(RTOS_TAG, ##__VA_ARGS__)
#define RTOS_LOGE(...) BK_LOGE(RTOS_TAG, ##__VA_ARGS__)
#define RTOS_LOGD(...) BK_LOGD(RTOS_TAG, ##__VA_ARGS__)

