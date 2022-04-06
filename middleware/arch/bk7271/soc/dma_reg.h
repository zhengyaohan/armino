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

#ifdef __cplusplus
extern "C" {
#endif

#define DMA_R_BASE                   (SOC_DMA_REG_BASE)

#define DMA_R_INT_STATUS             (DMA_R_BASE + (0x4 * 0x38))

#define DMA_F_FINI_INT_ST            (BIT(0))
#define DMA_F_FINI_INT_ST_M          (BIT(0))
#define DMA_F_FINI_INT_ST_V          0x3f
#define DMA_F_FINI_INT_ST_S          0
#define DMA_F_FINI_INT_ST_MS(_ch)    (DMA_F_FINI_INT_ST_S + (1 << (_ch)))

#define DMA_F_HF_FINI_INT_ST         (BIT(8))
#define DMA_F_HF_FINI_INT_ST_M       (BIT(8))
#define DMA_F_HF_FINI_INT_ST_V       0x3f
#define DMA_F_HF_FINI_INT_ST_S       8
#define DMA_F_HF_FINI_INT_ST_MS(_ch) (DMA_F_HF_FINI_INT_ST_S + (1 << (_ch)))

#define DMA_V_WORK_MODE_SINGLE       0x0
#define DMA_V_WORK_MODE_REPEAT       0x1

#define DMA_V_PRIO_MODE_ROUND_ROBIN      0x0
#define DMA_V_PRIO_MODE_FIXED_PRIO       0x1

#define DMA_V_REQ_MUX_DTCM           0x0
#define DMA_V_REQ_MUX_SDIO           0x2
#define DMA_V_REQ_MUX_JPEG           0x5
#define DMA_V_REQ_MUX_UART1          0x6
#define DMA_V_REQ_MUX_UART2          0x7
#define DMA_V_REQ_MUX_UART3          0x8
#define DMA_V_REQ_MUX_GSPI0          0x9
#define DMA_V_REQ_MUX_GSPI1          0xA
#define DMA_V_REQ_MUX_GSPI2          0xB

#ifdef __cplusplus
}
#endif


