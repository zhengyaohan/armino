// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "bk_log.h"

#define QSPI_TAG "qspi"
#define QSPI_LOGI(...) BK_LOGI(QSPI_TAG, ##__VA_ARGS__)
#define QSPI_LOGW(...) BK_LOGW(QSPI_TAG, ##__VA_ARGS__)
#define QSPI_LOGE(...) BK_LOGE(QSPI_TAG, ##__VA_ARGS__)
#define QSPI_LOGD(...) BK_LOGD(QSPI_TAG, ##__VA_ARGS__)
