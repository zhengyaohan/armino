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

#include "hal_config.h"
#include "dma_hw.h"
#include "dma_hal.h"
#include "dma_ll.h"

#if CFG_HAL_DEBUG_DMA

void dma_struct_dump(dma_id_t id)
{
	dma_hw_t *hw = (dma_hw_t *)DMA_LL_REG_BASE(0);
	SOC_LOGI("base=0x%x\r\n", (uint32_t)hw);

	SOC_LOGI("channel(%x)\r\n", id);

	SOC_LOGI("  ctrl addr=0x%x value=0x%x\n", &hw->config_group[id].ctrl, hw->config_group[id].ctrl.v);
	SOC_LOGI("    dma_en:                 %x\n", hw->config_group[id].ctrl.enable);
	SOC_LOGI("    finish_int_en:          %x\n", hw->config_group[id].ctrl.finish_int_en);
	SOC_LOGI("    half_finish_int_en:     %x\n", hw->config_group[id].ctrl.half_finish_int_en);
	SOC_LOGI("    dma_work_mode:          %x\n", hw->config_group[id].ctrl.mode);
	SOC_LOGI("    src_data_width:         %x\n", hw->config_group[id].ctrl.src_data_width);
	SOC_LOGI("    dest_data_width:        %x\n", hw->config_group[id].ctrl.dest_data_width);
	SOC_LOGI("    src_addr_increment_en:  %x\n", hw->config_group[id].ctrl.src_addr_inc_en);
	SOC_LOGI("    dest_addr_increment_en: %x\n", hw->config_group[id].ctrl.dest_addr_inc_en);
	SOC_LOGI("    src_addr_loop_en:       %x\n", hw->config_group[id].ctrl.src_addr_loop_en);
	SOC_LOGI("    dest_addr_loop_en:      %x\n", hw->config_group[id].ctrl.dest_addr_loop_en);
	SOC_LOGI("    channel_prioprity:      %x\n", hw->config_group[id].ctrl.chan_prio);
	SOC_LOGI("    reserved:               %x\n", hw->config_group[id].ctrl.reserved);
	SOC_LOGI("    transfer_len:           %x\n", hw->config_group[id].ctrl.transfer_len);

	SOC_LOGI("\n");
	SOC_LOGI("  dest_start_addr addr=0x%x value=0x%x\n", &hw->config_group[id].dest_start_addr, hw->config_group[id].dest_start_addr);
	SOC_LOGI("  src_start_addr addr=0x%x value=0x%x\n", &hw->config_group[id].src_start_addr, hw->config_group[id].src_start_addr);
	SOC_LOGI("  dest_loop_end_addr addr=0x%x value=0x%x\n", &hw->config_group[id].dest_loop_end_addr, hw->config_group[id].dest_loop_end_addr);
	SOC_LOGI("  dest_loop_start_addr addr=0x%x value=0x%x\n", &hw->config_group[id].dest_loop_start_addr, hw->config_group[id].dest_loop_start_addr);
	SOC_LOGI("  src_loop_end_addr addr=0x%x value=0x%x\n", &hw->config_group[id].src_loop_end_addr, hw->config_group[id].src_loop_end_addr);
	SOC_LOGI("  src_loop_start_addr addr=0x%x value=0x%x\n", &hw->config_group[id].src_loop_start_addr, hw->config_group[id].src_loop_start_addr);

	SOC_LOGI("\n");
	SOC_LOGI("  req_mux addr=0x%x value=0x%x\n", &hw->config_group[id].req_mux, hw->config_group[id].req_mux.v);
	SOC_LOGI("    source_request_mux:  %x\n", hw->config_group[id].req_mux.src_req_mux);
	SOC_LOGI("    dest_request_mux:    %x\n", hw->config_group[id].req_mux.dest_req_mux);
	SOC_LOGI("    reserved:            %x\n", hw->config_group[id].req_mux.reserved0);
	SOC_LOGI("    src_read_interval:   %x\n", hw->config_group[id].req_mux.src_read_interval);
	SOC_LOGI("    dest_write_interval: %x\n", hw->config_group[id].req_mux.dest_write_interval);
	SOC_LOGI("    reserved:            %x\n", hw->config_group[id].req_mux.reserved1);
	SOC_LOGI("\n");

	SOC_LOGI("channel(%x)\r\n", id);

	SOC_LOGI("  dma_status addr=0x%x value=0x%x\n", &hw->status_group[id].status, hw->status_group[id].status.v);
	SOC_LOGI("    remain_len_indication:   %x\n", hw->status_group[id].status.remain_len);
	SOC_LOGI("    flush_src_buff:          %x\n", hw->status_group[id].status.flush_src_buff);
	SOC_LOGI("    reserved:                %x\n", hw->status_group[id].status.reserved);
	SOC_LOGI("    finish_int_counter:      %x\n", hw->status_group[id].status.finish_int_counter);
	SOC_LOGI("    half_finish_int_counter: %x\n", hw->status_group[id].status.half_finish_int_counter);
	SOC_LOGI("\n");

	SOC_LOGI("  prio_mode addr=0x%x value=0x%x\n", &hw->prio_mode, hw->prio_mode.v);
	SOC_LOGI("    prio_mode: %x\n", hw->prio_mode.prio_mode);
	SOC_LOGI("    reserved:  %x\n", hw->prio_mode.reserved);
	SOC_LOGI("\n");

	SOC_LOGI("  int_status addr=0x%x value=0x%x\n", &hw->int_status, hw->int_status.v);
	SOC_LOGI("    finish_int_status:      %x\n", hw->int_status.finish_int_status);
	SOC_LOGI("    half_finish_int_status: %x\n", hw->int_status.half_finish_int_status);
	SOC_LOGI("\n");

	SOC_LOGI("  DMA%x source_pause_addr addr=0x%x value=0x%x\n", id, &hw->src_pause_addr[id], hw->src_pause_addr[id]);
	SOC_LOGI("  DMA%x dest_pause_addr addr=0x%x value=0x%x\n", id, &hw->dest_pause_addr[id], hw->dest_pause_addr[id]);
	SOC_LOGI("  DMA%x source_read_addr addr=0x%x value=0x%x\n", id, &hw->src_rd_addr[id], hw->src_rd_addr[id]);
	SOC_LOGI("  DMA%x dest_write_addr addr=0x%x value=0x%x\n", id, &hw->dest_wr_addr[id], hw->dest_wr_addr[id]);
}

#endif

