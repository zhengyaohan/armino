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

#include "rtos_pub.h"
#include "cli.h"
#include "bk_api_aud.h"
#include "bk_api_aud_types.h"
#include "bk_api_dma.h"
#include "aud_hal.h"
#include "sys_driver.h"
#include "fft_driver.h"

typedef enum {
	ADC_TEST_MODE_NULL = 0,
	ADC_TEST_MODE_MCP,
	ADC_TEST_MODE_DMA,
	ADC_TEST_MODE_MAX
} aud_test_mode_t;

static const unsigned long PCM_8000[] = {
	0x00010000, 0x5A825A81, 0x7FFF7FFF, 0x5A825A83, 0x00000000, 0xA57FA57E, 0x80018002, 0xA57EA57E,
};

static const unsigned long PCM_16000[] = {
	0x00000001, 0x30FC30FA, 0x5A815A82, 0x76417641, 0x7FFF7FFF, 0x76417641, 0x5A815A82, 0x30FB30FB,
	0x00000000, 0xCF05CF04, 0xA57EA57E, 0x89BF89BF, 0x80018000, 0x89C089BF, 0xA57EA57F, 0xCF05CF04,
};

static const unsigned long PCM_44100[] = {
	0x00000000, 0x122D122E, 0x23FC23FB, 0x350F350F, 0x450F4510, 0x53AA53AA, 0x60926092, 0x6B866B85,
	0x744A744A, 0x7AB57AB5, 0x7EA37EA2, 0x7FFF7FFF, 0x7EC37EC4, 0x7AF77AF6, 0x74AB74AC, 0x6C036C04,
	0x612B612A, 0x5459545A, 0x45D445D3, 0x35E335E3, 0x24DB24DA, 0x13141313, 0x00EA00EA, 0xEEBAEEBB,
	0xDCE5DCE4, 0xCBC5CBC6, 0xBBB6BBB5, 0xAD08AD08, 0xA008A008, 0x94FA94FA, 0x8C188C17, 0x858E858E,
	0x81818181, 0x80038003, 0x811D811D, 0x84C984CA, 0x8AF58AF5, 0x93809381, 0x9E3E9E3E, 0xAAF7AAF7,
	0xB969B96A, 0xC94AC94A, 0xDA46DA46, 0xEC05EC06, 0xFE2DFE2E, 0x105E105E, 0x223B223B, 0x33653365,
	0x43854384, 0x52465246, 0x5F5D5F5D, 0x6A856A85, 0x73847384, 0x7A2C7A2C, 0x7E5B7E5B, 0x7FFA7FFA,
	0x7F007F01, 0x7B747B75, 0x75697569, 0x6CFB6CFB, 0x62586258, 0x55B755B7, 0x47594759, 0x37893789,
	0x26992699, 0x14E114E1, 0x02BC02BC, 0xF08AF08A, 0xDEA6DEA6, 0xCD72CD72, 0xBD42BD43, 0xAE6DAE6D,
	0xA13FA13F, 0x95FD95FD, 0x8CE18CE2, 0x86198619, 0x81CA81CB, 0x800B800C, 0x80E380E3, 0x844E844E,
	0x8A3C8A3C, 0x928B928B, 0x9D149D13, 0xA99CA99C, 0xB7E6B7E6, 0xC7A4C7A5, 0xD889D888, 0xEA39EA39,
	0xFC5AFC5A, 0x0E8F0E8F, 0x20782077, 0x31B831B8, 0x41F641F6, 0x50DF50DF, 0x5E235E23, 0x697F697F,
	0x72B972B8, 0x799F799E, 0x7E0D7E0D, 0x7FEE7FEE, 0x7F387F38, 0x7BED7BED, 0x761F761F, 0x6DEC6DED,
	0x63806380, 0x570F5710, 0x48DB48DA, 0x392C392C, 0x28552855, 0x16AC16AD, 0x048F048F, 0xF25AF259,
	0xE06BE06B, 0xCF1FCF1F, 0xBED2BED3, 0xAFD8AFD7, 0xA27CA27B, 0x97059706, 0x8DB08DB1, 0x86AB86AB,
	0x821C821C, 0x801A801A, 0x80B080AF, 0x83DA83DA, 0x89888988, 0x919C919C, 0x9BED9BEE, 0xA847A846,
	0xB666B666, 0xC603C603, 0xD6CED6CD, 0xE86DE86D, 0xFA89FA87, 0x0CBE0CBF, 0x1EB31EB3, 0x30083007,
	0x40644064, 0x4F734F73, 0x5CE55CE4, 0x68756874, 0x71E771E7, 0x790A790A, 0x7DB87DB9, 0x7FDC7FDD,
	0x7F677F68, 0x7C5F7C5E, 0x76CF76D0, 0x6ED96EDA, 0x64A364A3, 0x58635863, 0x4A594A58, 0x3ACD3ACC,
	0x2A102A0F, 0x18781878, 0x06610661, 0xF429F42A, 0xE230E22F, 0xD0D0D0D0, 0xC066C066, 0xB145B145,
	0xA3BCA3BD, 0x98149813, 0x8E848E85, 0x87438743, 0x82738273, 0x802F8030, 0x80848083, 0x836B836B,
	0x88DA88DB, 0x90B390B3, 0x9ACE9ACD, 0xA6F5A6F5, 0xB4EAB4EA, 0xC464C465, 0xD515D515, 0xE6A3E6A3,
	0xF8B6F8B6, 0x0AED0AEE, 0x1CEC1CED, 0x2E562E57, 0x3ED03ED0, 0x4E024E03, 0x5BA15BA0, 0x67646764,
	0x710E710F, 0x786F786E, 0x7D5E7D5E, 0x7FC37FC3, 0x7F917F91, 0x7CCA7CC9, 0x777A777A, 0x6FBF6FC0,
	0x65C165C2, 0x59B259B3, 0x4BD34BD3, 0x3C693C69, 0x2BC62BC7, 0x1A411A40, 0x08330834, 0xF5FBF5FB,
	0xE3F6E3F7, 0xD283D284, 0xC1FCC1FD, 0xB2B7B2B8, 0xA503A503, 0x99279926, 0x8F608F60, 0x87E187E2,
	0x82D282D2, 0x804D804C, 0x805D805C, 0x83038304, 0x88338833, 0x8FCF8FCF, 0x99B299B1, 0xA5A8A5A8,
	0xB371B372, 0xC2C8C2CA, 0xD35ED35E, 0xE4DAE4DB, 0xF6E4F6E4, 0x091C091C, 0x1B261B25, 0x2CA22CA2,
	0x3D373D37, 0x4C8E4C8E, 0x5A595A59, 0x664E664E, 0x70307030, 0x77CD77CE, 0x7CFD7CFD, 0x7FA37FA3,
	0x7FB37FB4, 0x7D2E7D2E, 0x781F781F, 0x70A170A0, 0x66DA66DA, 0x5AFE5AFD, 0x4D494D49, 0x3E033E04,
	0x2D7D2D7D, 0x1C091C0A, 0x0A050A05, 0xF7CCF7CD, 0xE5BEE5BE, 0xD439D439, 0xC396C396, 0xB42DB42D,
	0xA64DA64E, 0x9A3E9A3E, 0x90409040, 0x88868886, 0x83378337, 0x806E806E, 0x803D803E, 0x82A382A1,
	0x87928791, 0x8EF18EF2, 0x989C989C, 0xA45FA45F, 0xB1FEB1FD, 0xC131C131, 0xD1AAD1AA, 0xE313E313,
	0xF512F512, 0x074A074B, 0x195D195C, 0x2AEC2AEC, 0x3B9C3B9C, 0x4B164B16, 0x590C590B, 0x65336532,
	0x6F4D6F4C, 0x77267726, 0x7C957C95, 0x7F7D7F7D, 0x7FD17FD1, 0x7D8C7D8C, 0x78BD78BC, 0x717B717A,
	0x67ED67ED, 0x5C445C43, 0x4EBB4EBB, 0x3F9B3F9A, 0x2F302F30, 0x1DD01DD0, 0x0BD60BD7, 0xF99FF99E,
	0xE788E789, 0xD5F1D5F1, 0xC533C533, 0xB5A8B5A7, 0xA79DA79D, 0x9B5D9B5D, 0x91289127, 0x89308930,
	0x83A283A2, 0x80988098, 0x80248024, 0x82468247, 0x86F786F6, 0x8E1A8E19, 0x978C978C, 0xA31CA31C,
	0xB08EB08D, 0xBF9CBF9B, 0xCFF7CFF8, 0xE14DE14C, 0xF342F342, 0x05780578, 0x17931792, 0x29332933,
	0x39FE39FD, 0x499B499A, 0x57B957BA, 0x64126413, 0x6E636E64, 0x76787678, 0x7C277C27, 0x7F517F50,
	0x7FE67FE5, 0x7DE47DE3, 0x79557955, 0x72517250, 0x68FA68FB, 0x5D855D84, 0x50295029, 0x412E412E,
	0x30E030E1, 0x1F961F95, 0x0DA70DA6, 0xFB71FB70, 0xE954E953, 0xD7ABD7AB, 0xC6D4C6D3, 0xB726B725,
	0xA8F1A8F1, 0x9C809C80, 0x92139213, 0x89E189E0, 0x84138413, 0x80C880C9, 0x80128011, 0x81F381F3,
	0x86618663, 0x8D478D48, 0x96819680, 0xA1DDA1DD, 0xAF22AF22, 0xBE0ABE0A, 0xCE48CE48, 0xDF88DF89,
	0xF172F171, 0x03A603A5, 0x15C715C8, 0x27782777, 0x385B385B, 0x481B481A, 0x56635664, 0x62ED62EC,
	0x6D746D75, 0x75C475C5, 0x7BB17BB2, 0x7F1C7F1C, 0x7FF57FF4, 0x7E347E35, 0x79E679E6, 0x731F731F,
	0x6A046A03, 0x5EC15EC1, 0x51935194, 0x42BE42BE, 0x328E328F, 0x21592159, 0x0F770F77, 0xFD43FD44,
	0xEB20EB1F, 0xD967D966, 0xC877C877, 0xB8A8B8A7, 0xAA49AA49, 0x9DA89DA9, 0x93059306, 0x8A988A98,
	0x848B848B, 0x80FF80FF, 0x80068006, 0x81A581A4, 0x85D485D3, 0x8C7B8C7C, 0x957B957A, 0xA0A4A0A2,
	0xADBAADBA, 0xBC7BBC7B, 0xCC9CCC9C, 0xDDC6DDC6, 0xEFA2EFA2, 0x01D201D3, 0x13FB13FB, 0x25BA25BA,
	0x36B636B7, 0x46964697, 0x550A5509, 0x61C261C2, 0x6C806C7F, 0x750B750A, 0x7B377B36, 0x7EE37EE2,
	0x7FFD7FFE, 0x7E7F7E80, 0x7A717A71, 0x73E973E8, 0x6B066B06, 0x5FF85FF8, 0x52F952F8, 0x444A444B,
	0x343B343A, 0x231B231A, 0x11461146, 0xFF17FF17, 0xECEDECEC, 0xDB26DB25, 0xCA1DCA1E, 0xBA2CBA2D,
	0xABA6ABA6, 0x9ED59ED5, 0x93FD93FD, 0x8B558B55, 0x85098509, 0x813C813D, 0x80018001, 0x815E815E,
	0x854B854C, 0x8BB58BB5, 0x947B947B, 0x9F6D9F6F, 0xAC56AC57, 0xBAF1BAF0, 0xCAF1CAF1, 0xDC05DC04,
	0xEDD2EDD4,
};

static const unsigned long PCM_48000[] = {
	0x0000FFFF, 0x10B510B5, 0x21202121, 0x30FB30FC, 0x40003FFF, 0x4DEC4DEC, 0x5A825A82, 0x658C658C,
	0x6EDA6ED9, 0x76417641, 0x7BA37BA2, 0x7EE77EE7, 0x7FFF7FFF, 0x7EE67EE6, 0x7BA27BA3, 0x76417641,
	0x6ED86EDA, 0x658C658B, 0x5A825A82, 0x4DEB4DEB, 0x3FFF3FFF, 0x30FC30FB, 0x21202120, 0x10B510B5,
	0x00010000, 0xEF4AEF4B, 0xDEDFDEDF, 0xCF05CF05, 0xC000C000, 0xB214B215, 0xA57EA57F, 0x9A759A74,
	0x91269127, 0x89C089BF, 0x845D845D, 0x811A8119, 0x80018001, 0x811A811A, 0x845E845D, 0x89BF89C0,
	0x91279127, 0x9A749A74, 0xA57EA57E, 0xB214B216, 0xC001C001, 0xCF05CF04, 0xDEDFDEDF, 0xEF4BEF4B,
};

#define test_size  2*1024
uint32_t pcm_data[test_size] = {0};

static aud_test_mode_t adc_test_mode = ADC_TEST_MODE_NULL;

extern void delay(int num);//TODO fix me

static void cli_aud_help(void)
{
	CLI_LOGI("aud_adc_mcp_test {start|stop} \r\n");
	CLI_LOGI("aud_dtmf_mcp_test {start|stop} \r\n");
	CLI_LOGI("aud_adc_dma_test {start|stop} \r\n");
	CLI_LOGI("aud_dtmf_dma_test {start|stop} \r\n");
	CLI_LOGI("aud_adc_loop_test {start|stop} \r\n");
	CLI_LOGI("aud_dtmf_loop_test {start|stop} \r\n");
	CLI_LOGI("aud_pcm_mcp_test {8000|16000|44100|48000} \r\n");
	CLI_LOGI("aud_pcm_dma_test {8000|16000|44100|48000|stop} \r\n");
	CLI_LOGI("aud_adc_to_mcp_test {start|stop} \r\n");
}

static void cli_aud_adcl_isr(void *param)
{
	uint32_t adc_data;
	aud_adc_status_t adc_status;

	if (adc_test_mode == ADC_TEST_MODE_MCP) {
		bk_aud_get_adc_status(&adc_status);
		if (adc_status.adcl_near_full) {
			bk_aud_get_adc_fifo_data(&adc_data);
			bk_aud_dac_write(adc_data);
		}
	}
}

static void cli_aud_dtmf_isr( void *param)
{
	uint32_t dtmf_data;
	aud_adc_status_t adc_status;

	if (adc_test_mode == ADC_TEST_MODE_MCP) {
		bk_aud_get_adc_status(&adc_status);
		if (adc_status.dtmf_near_full || adc_status.dtmf_fifo_full) {
			bk_aud_get_dtmf_fifo_data(&dtmf_data);
			bk_aud_dac_write(dtmf_data);
		}
	}
}

static void cli_aud_adc_dma_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_adc_config_t adc_config;
	uint32_t adc_fifo_addr;
	dma_config_t dma_config;
	uint32_t dac_fifo_addr;
	aud_dac_config_t dac_config;

	if (argc != 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("audio adc dma test start\n");
		adc_test_mode = ADC_TEST_MODE_DMA;
		adc_config.samp_rate = AUD_ADC_SAMP_RATE_8K;
		adc_config.adc_enable = AUD_ADC_DISABLE;
		adc_config.line_enable = AUD_ADC_LINE_DISABLE;
		adc_config.dtmf_enable = AUD_DTMF_DISABLE;
		adc_config.adc_hpf2_coef_B2 = 0;
		adc_config.adc_hpf2_bypass_enable = AUD_ADC_HPF_BYPASS_ENABLE;
		adc_config.adc_hpf1_bypass_enable = AUD_ADC_HPF_BYPASS_ENABLE;
		adc_config.adc_set_gain = 0x2d;
		adc_config.adc_samp_edge = AUD_ADC_SAMP_EDGE_RISING;
		adc_config.adc_hpf2_coef_B0 = 0;
		adc_config.adc_hpf2_coef_B1 = 0;
		adc_config.adc_hpf2_coef_A0 = 0;
		adc_config.adc_hpf2_coef_A1 = 0;
		adc_config.dtmf_wr_threshold = 8;
		adc_config.adcl_wr_threshold = 8;
		adc_config.dtmf_int_enable = AUD_DTMF_INT_DISABLE;
		adc_config.adcl_int_enable = AUD_ADCL_INT_DISABLE;
		adc_config.loop_adc2dac = AUD_LOOP_ADC2DAC_DISABLE;
		adc_config.agc_noise_thrd = 101;
		adc_config.agc_noise_high = 101;
		adc_config.agc_noise_low = 160;
		adc_config.agc_noise_min = 1;
		adc_config.agc_noise_tout = 0;
		adc_config.agc_high_dur = 3;
		adc_config.agc_low_dur = 3;
		adc_config.agc_min = 1;
		adc_config.agc_max = 4;
		adc_config.agc_ng_method = AUD_AGC_NG_METHOD_MUTE;
		adc_config.agc_ng_enable = AUD_AGC_NG_DISABLE;
		adc_config.agc_decay_time = AUD_AGC_DECAY_TIME_128;
		adc_config.agc_attack_time = AUD_AGC_ATTACK_TIME_128;
		adc_config.agc_high_thrd = 18;
		adc_config.agc_low_thrd = 0;
		adc_config.agc_iir_coef = AUD_AGC_IIR_COEF_1_1024;
		adc_config.agc_enable = AUD_AGC_DISABLE;
		adc_config.manual_pga_value = 0;
		adc_config.manual_pga_enable = AUD_GAC_MANUAL_PGA_DISABLE;
		adc_config.adc_fracmod_manual = AUD_ADC_TRACMOD_MANUAL_DISABLE;
		adc_config.adc_fracmod = 0;

		dac_config.dac_enable = AUD_DAC_DISABLE;
		dac_config.samp_rate = AUD_DAC_SAMP_RATE_8K;
		dac_config.dac_hpf2_coef_B2 = 0x3A22;
		dac_config.dac_hpf2_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
		dac_config.dac_hpf1_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
		dac_config.dac_set_gain = 0x2D;
		dac_config.dac_clk_invert = AUD_DAC_CLK_INVERT_RISING;

		dac_config.dac_hpf2_coef_B0 = 0x3A22;
		dac_config.dac_hpf2_coef_B1 = 0x8BBF;

		dac_config.dac_hpf2_coef_A1 = 0x751C;
		dac_config.dac_hpf2_coef_A2 = 0xC9E6;

		dac_config.dacr_rd_threshold = 0x4;
		dac_config.dacl_rd_threshold = 0x4;
		dac_config.dacr_int_enable = 0x0;
		dac_config.dacl_int_enable = 0x0;

		dac_config.dac_filt_enable = AUD_DAC_FILT_DISABLE;
		dac_config.dac_fracmod_manual_enable = AUD_DAC_FRACMOD_MANUAL_DISABLE;
		dac_config.dac_fracmode_value = 0x0;

		//init audio driver
		bk_aud_driver_init();

		bk_aud_adc_init(AUD_ADC_WORK_MODE_ADC, &adc_config, NULL);
		bk_aud_dac_init(&dac_config);
		CLI_LOGI("init audio driver, adc and dac successful\n");

		//init dma driver
		ret = bk_dma_driver_init();
		if (ret != BK_OK) {
			CLI_LOGE("dma driver init failed\r\n");
			return;
		}
		dma_config.mode = DMA_WORK_MODE_REPEAT;
		dma_config.chan_prio = 1;
		dma_config.src.dev = DMA_DEV_AUDIO;
		dma_config.src.width = DMA_DATA_WIDTH_32BITS;
		dma_config.dst.dev = DMA_DEV_AUDIO;
		dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
		//get adc fifo address
		if (bk_aud_get_adc_fifo_addr(&adc_fifo_addr) != BK_OK) {
			CLI_LOGE("get adc fifo address failed\r\n");
			return;
		} else {
			dma_config.src.start_addr = adc_fifo_addr;
			dma_config.src.end_addr = adc_fifo_addr + 4;
		}
		//get dac fifo address
		if (bk_aud_get_dac_fifo_addr(&dac_fifo_addr) != BK_OK) {
			CLI_LOGE("get dac fifo address failed\r\n");
			return;
		} else {
			dma_config.dst.start_addr = dac_fifo_addr;
			dma_config.dst.end_addr = dac_fifo_addr + 4;
		}

		os_printf("source_addr:0x%x, dest_addr:0x%x\r\n", dma_config.src.start_addr, dma_config.dst.start_addr);

		//init dma channel
		ret = bk_dma_init(DMA_ID_2, &dma_config);
		if (ret != BK_OK) {
			CLI_LOGE("dma init failed\r\n");
			return;
		}
		bk_dma_set_transfer_len(DMA_ID_2, 4);
		ret = bk_dma_start(DMA_ID_2);

		//start adc and dac
		bk_aud_start_adc();
		bk_aud_start_dac();
		CLI_LOGI("enable adc and dac successful\n");

		//aud_struct_dump();

		if (ret == BK_OK)
			CLI_LOGI("start audio adc test successful\r\n");
		else
			CLI_LOGE("start audio adc test failed\r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		CLI_LOGI("audio adc test stop\n");
		adc_test_mode = ADC_TEST_MODE_NULL;
		//disable adc and dac
		bk_aud_stop_adc();
		ret = bk_aud_driver_deinit();
		if (ret != BK_OK)
			return;
		//stop dma
		bk_dma_stop(DMA_ID_2);
		bk_dma_deinit(DMA_ID_2);
		if (ret != BK_OK)
			return;
		CLI_LOGI("audio adc test stop successful\n");
	} else {
		cli_aud_help();
		return;
	}
}

static void cli_aud_dtmf_dma_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_dtmf_config_t dtmf_config;
	uint32_t dtmf_fifo_addr;
	dma_config_t dma_config;
	uint32_t dac_fifo_addr;
	aud_dac_config_t dac_config;

	if (argc != 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("audio dtmf test start\n");
		adc_test_mode = ADC_TEST_MODE_DMA;
		dtmf_config.adc_enable = AUD_ADC_DISABLE;
		dtmf_config.line_enable = AUD_ADC_LINE_DISABLE;
		dtmf_config.dtmf_enable = AUD_DTMF_DISABLE;
		dtmf_config.tone_pattern = AUD_DTMF_TONE_PATTERN_DTMF;
		dtmf_config.tone_mode = AUD_DTMF_TONE_MODE_CONTIUS;
		dtmf_config.tone_pause_time = 2;
		dtmf_config.tone_active_time = 3;
		dtmf_config.tone1_step = 10240;
		dtmf_config.tone1_attu = AUD_DTMF_TONE_ATTU_MINUS_1;
		dtmf_config.tone1_enable = AUD_DTMF_TONE_ENABLE;
		dtmf_config.tone2_step = 5734;
		dtmf_config.tone1_attu = AUD_DTMF_TONE_ATTU_MINUS_1;
		dtmf_config.tone1_enable = AUD_DTMF_TONE_ENABLE;
		dtmf_config.dtmf_wr_threshold = 8;
		dtmf_config.dtmf_int_enable = AUD_DTMF_INT_DISABLE;
		dtmf_config.loop_ton2dac = AUD_LOOP_DTMF2DAC_DISABLE;

		dac_config.dac_enable = AUD_DAC_DISABLE;
		dac_config.samp_rate = AUD_DAC_SAMP_RATE_8K;
		dac_config.dac_hpf2_coef_B2 = 0x3A22;
		dac_config.dac_hpf2_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
		dac_config.dac_hpf1_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
		dac_config.dac_set_gain = 0x2D;
		dac_config.dac_clk_invert = AUD_DAC_CLK_INVERT_RISING;

		dac_config.dac_hpf2_coef_B0 = 0x3A22;
		dac_config.dac_hpf2_coef_B1 = 0x8BBF;

		dac_config.dac_hpf2_coef_A1 = 0x751C;
		dac_config.dac_hpf2_coef_A2 = 0xC9E6;

		dac_config.dacr_rd_threshold = 0x4;
		dac_config.dacl_rd_threshold = 0x4;
		dac_config.dacr_int_enable = 0x0;
		dac_config.dacl_int_enable = 0x0;

		dac_config.dac_filt_enable = AUD_DAC_FILT_DISABLE;
		dac_config.dac_fracmod_manual_enable = AUD_DAC_FRACMOD_MANUAL_DISABLE;
		dac_config.dac_fracmode_value = 0x0;

		//init audio driver
		ret = bk_aud_driver_init();
		if (ret != BK_OK)
			return;
		bk_aud_adc_init(AUD_ADC_WORK_MODE_DTMF, NULL, &dtmf_config);
		bk_aud_dac_init(&dac_config);
		CLI_LOGI("init audio driver, adc and dac successful\n");

		//init dma driver
		ret = bk_dma_driver_init();
		if (ret != BK_OK) {
			CLI_LOGE("dma driver init failed\r\n");
			return;
		}
		dma_config.mode = DMA_WORK_MODE_REPEAT;
		dma_config.chan_prio = 1;
		dma_config.src.dev = DMA_DEV_AUDIO;
		dma_config.src.width = DMA_DATA_WIDTH_32BITS;
		dma_config.dst.dev = DMA_DEV_AUDIO;
		dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
		//get adc fifo address
		if (bk_aud_get_dtmf_fifo_addr(&dtmf_fifo_addr) != BK_OK) {
			CLI_LOGE("get adc fifo address failed\r\n");
			return;
		} else {
			dma_config.src.start_addr = dtmf_fifo_addr;
			dma_config.src.end_addr = dtmf_fifo_addr + 4;
		}
		//get dac fifo address
		if (bk_aud_get_dac_fifo_addr(&dac_fifo_addr) != BK_OK) {
			CLI_LOGE("get dac fifo address failed\r\n");
			return;
		} else {
			dma_config.dst.start_addr = dac_fifo_addr;
			dma_config.dst.end_addr = dac_fifo_addr + 4;
		}

		os_printf("source_addr:0x%x, dest_addr:0x%x\r\n", dma_config.src.start_addr, dma_config.dst.start_addr);

		//init dma channel
		ret = bk_dma_init(DMA_ID_2, &dma_config);
		if (ret != BK_OK) {
			CLI_LOGE("dma init failed\r\n");
			return;
		}
		bk_dma_set_transfer_len(DMA_ID_2, 4);
		ret = bk_dma_start(DMA_ID_2);

		//start adc and dac
		bk_aud_start_adc();
		bk_aud_start_dac();
		CLI_LOGI("enable adc and dac successful\n");

		//aud_struct_dump();

		if (ret == BK_OK)
			CLI_LOGI("start audio adc test successful\r\n");
		else
			CLI_LOGE("start audio adc test failed\r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		CLI_LOGI("audio dtmf test stop\n");
		adc_test_mode = ADC_TEST_MODE_NULL;
		//disable adc and dac
		bk_aud_stop_adc();
		ret = bk_aud_driver_deinit();
		if (ret != BK_OK)
			return;
		//stop dma
		bk_dma_stop(DMA_ID_2);
		bk_dma_deinit(DMA_ID_2);
		if (ret != BK_OK)
			return;
		CLI_LOGI("audio dtmf test stop successful\n");
	} else {
		cli_aud_help();
		return;
	}
}

static void cli_aud_adc_mcp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_adc_config_t adc_config;
	aud_dac_config_t dac_config;

	if (argc != 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("audio adc mcp test start\n");
		adc_test_mode = ADC_TEST_MODE_MCP;
		adc_config.samp_rate = AUD_ADC_SAMP_RATE_8K;
		adc_config.adc_enable = AUD_ADC_DISABLE;
		adc_config.line_enable = AUD_ADC_LINE_DISABLE;
		adc_config.dtmf_enable = AUD_DTMF_DISABLE;
		adc_config.adc_hpf2_coef_B2 = 0;
		adc_config.adc_hpf2_bypass_enable = AUD_ADC_HPF_BYPASS_ENABLE;
		adc_config.adc_hpf1_bypass_enable = AUD_ADC_HPF_BYPASS_ENABLE;
		adc_config.adc_set_gain = 0x2d;
		adc_config.adc_samp_edge = AUD_ADC_SAMP_EDGE_RISING;
		adc_config.adc_hpf2_coef_B0 = 0;
		adc_config.adc_hpf2_coef_B1 = 0;
		adc_config.adc_hpf2_coef_A0 = 0;
		adc_config.adc_hpf2_coef_A1 = 0;
		adc_config.dtmf_wr_threshold = 8;
		adc_config.adcl_wr_threshold = 8;
		adc_config.dtmf_int_enable = AUD_DTMF_INT_DISABLE;
		adc_config.adcl_int_enable = AUD_ADCL_INT_DISABLE;
		adc_config.loop_adc2dac = AUD_LOOP_ADC2DAC_DISABLE;
		adc_config.agc_noise_thrd = 101;
		adc_config.agc_noise_high = 101;
		adc_config.agc_noise_low = 160;
		adc_config.agc_noise_min = 1;
		adc_config.agc_noise_tout = 0;
		adc_config.agc_high_dur = 3;
		adc_config.agc_low_dur = 3;
		adc_config.agc_min = 1;
		adc_config.agc_max = 4;
		adc_config.agc_ng_method = AUD_AGC_NG_METHOD_MUTE;
		adc_config.agc_ng_enable = AUD_AGC_NG_DISABLE;
		adc_config.agc_decay_time = AUD_AGC_DECAY_TIME_128;
		adc_config.agc_attack_time = AUD_AGC_ATTACK_TIME_128;
		adc_config.agc_high_thrd = 18;
		adc_config.agc_low_thrd = 0;
		adc_config.agc_iir_coef = AUD_AGC_IIR_COEF_1_1024;
		adc_config.agc_enable = AUD_AGC_DISABLE;
		adc_config.manual_pga_value = 0;
		adc_config.manual_pga_enable = AUD_GAC_MANUAL_PGA_DISABLE;
		adc_config.adc_fracmod_manual = AUD_ADC_TRACMOD_MANUAL_DISABLE;
		adc_config.adc_fracmod = 0;

		dac_config.dac_enable = AUD_DAC_DISABLE;
		dac_config.samp_rate = AUD_DAC_SAMP_RATE_8K;
		dac_config.dac_hpf2_coef_B2 = 0x3A22;
		dac_config.dac_hpf2_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
		dac_config.dac_hpf1_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
		dac_config.dac_set_gain = 0x2D;
		dac_config.dac_clk_invert = AUD_DAC_CLK_INVERT_RISING;

		dac_config.dac_hpf2_coef_B0 = 0x3A22;
		dac_config.dac_hpf2_coef_B1 = 0x8BBF;

		dac_config.dac_hpf2_coef_A1 = 0x751C;
		dac_config.dac_hpf2_coef_A2 = 0xC9E6;

		dac_config.dacr_rd_threshold = 0x4;
		dac_config.dacl_rd_threshold = 0x4;
		dac_config.dacr_int_enable = 0x0;
		dac_config.dacl_int_enable = 0x0;

		dac_config.dac_filt_enable = AUD_DAC_FILT_DISABLE;
		dac_config.dac_fracmod_manual_enable = AUD_DAC_FRACMOD_MANUAL_DISABLE;
		dac_config.dac_fracmode_value = 0x0;

		//init audio driver
		bk_aud_driver_init();

		bk_aud_adc_init(AUD_ADC_WORK_MODE_ADC, &adc_config, NULL);
		bk_aud_dac_init(&dac_config);
		CLI_LOGI("init audio driver, adc and dac successful\n");

		//register isr
		ret = bk_aud_register_aud_isr(AUD_ISR_ADCL, cli_aud_adcl_isr, NULL);
		if (ret != BK_OK)
			return;
		CLI_LOGI("register adc isr successful\n");

		//enable audio interrupt
		bk_aud_enable_adc_int();
		CLI_LOGI("enable adc and dac interrupt successful\n");

		//start adc and dac
		bk_aud_start_adc();
		bk_aud_start_dac();
		CLI_LOGI("enable adc and dac successful\n");

		//aud_struct_dump();

		CLI_LOGI("start audio adc mcp test successful\r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		CLI_LOGI("audio adc mcp test stop\n");
		bk_aud_driver_deinit();
		adc_test_mode = ADC_TEST_MODE_NULL;
		CLI_LOGI("audio adc mcp test stop successful\n");
	} else {
		cli_aud_help();
		return;
	}
}

static void cli_aud_dtmf_mcp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_dtmf_config_t dtmf_config;
	aud_dac_config_t dac_config;

	if (argc != 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("audio dtmf mcp test start\n");
		adc_test_mode = ADC_TEST_MODE_MCP;
		dtmf_config.adc_enable = AUD_ADC_DISABLE;
		dtmf_config.line_enable = AUD_ADC_LINE_DISABLE;
		dtmf_config.dtmf_enable = AUD_DTMF_DISABLE;
		dtmf_config.tone_pattern = AUD_DTMF_TONE_PATTERN_DTMF;
		dtmf_config.tone_mode = AUD_DTMF_TONE_MODE_CONTIUS;
		dtmf_config.tone_pause_time = 2;
		dtmf_config.tone_active_time = 4;
		dtmf_config.tone1_step = 10240;
		dtmf_config.tone1_attu = AUD_DTMF_TONE_ATTU_MINUS_1;
		dtmf_config.tone1_enable = AUD_DTMF_TONE_ENABLE;
		dtmf_config.tone2_step = 5734;
		dtmf_config.tone1_attu = AUD_DTMF_TONE_ATTU_MINUS_1;
		dtmf_config.tone1_enable = AUD_DTMF_TONE_ENABLE;
		dtmf_config.dtmf_wr_threshold = 8;
		dtmf_config.dtmf_int_enable = AUD_DTMF_INT_DISABLE;
		dtmf_config.loop_ton2dac = AUD_LOOP_DTMF2DAC_DISABLE;

		dac_config.dac_enable = AUD_DAC_DISABLE;
		dac_config.samp_rate = AUD_DAC_SAMP_RATE_8K;
		dac_config.dac_hpf2_coef_B2 = 0x3A22;
		dac_config.dac_hpf2_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
		dac_config.dac_hpf1_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
		dac_config.dac_set_gain = 0x2D;
		dac_config.dac_clk_invert = AUD_DAC_CLK_INVERT_RISING;

		dac_config.dac_hpf2_coef_B0 = 0x3A22;
		dac_config.dac_hpf2_coef_B1 = 0x8BBF;

		dac_config.dac_hpf2_coef_A1 = 0x751C;
		dac_config.dac_hpf2_coef_A2 = 0xC9E6;

		dac_config.dacr_rd_threshold = 0x4;
		dac_config.dacl_rd_threshold = 0x4;
		dac_config.dacr_int_enable = 0x0;
		dac_config.dacl_int_enable = 0x0;

		dac_config.dac_filt_enable = AUD_DAC_FILT_DISABLE;
		dac_config.dac_fracmod_manual_enable = AUD_DAC_FRACMOD_MANUAL_DISABLE;
		dac_config.dac_fracmode_value = 0x0;

		//init audio driver
		bk_aud_driver_init();

		bk_aud_adc_init(AUD_ADC_WORK_MODE_DTMF, NULL, &dtmf_config);
		bk_aud_dac_init(&dac_config);
		CLI_LOGI("init audio driver, adc and dac successful\n");

		//register isr
		ret = bk_aud_register_aud_isr(AUD_ISR_DTMF, cli_aud_dtmf_isr, NULL);
		if (ret != BK_OK)
			return;
		CLI_LOGI("register adc isr successful\n");

		//enable audio interrupt
		bk_aud_enable_adc_int();
		CLI_LOGI("enable adc and dac interrupt successful\n");

		//start adc and dac
		bk_aud_start_adc();
		bk_aud_start_dac();
		CLI_LOGI("enable adc and dac successful\n");

		//aud_struct_dump();

		CLI_LOGI("start audio adc mcp test successful\r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		CLI_LOGI("audio adc mcp test stop\n");
		bk_aud_driver_deinit();
		adc_test_mode = ADC_TEST_MODE_NULL;
		CLI_LOGI("audio adc mcp test stop successful\n");
	} else {
		cli_aud_help();
		return;
	}
}


static void cli_aud_adc_loop_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_adc_config_t adc_config;
	aud_dac_config_t dac_config;

	if (argc != 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("audio adc loop test start\n");
		adc_config.samp_rate = AUD_ADC_SAMP_RATE_16K;
		adc_config.adc_enable = AUD_ADC_DISABLE;
		adc_config.line_enable = AUD_ADC_LINE_DISABLE;
		adc_config.dtmf_enable = AUD_DTMF_DISABLE;
		adc_config.adc_hpf2_coef_B2 = 0;
		adc_config.adc_hpf2_bypass_enable = AUD_ADC_HPF_BYPASS_ENABLE;
		adc_config.adc_hpf1_bypass_enable = AUD_ADC_HPF_BYPASS_ENABLE;
		adc_config.adc_set_gain = 0x2d;
		adc_config.adc_samp_edge = AUD_ADC_SAMP_EDGE_RISING;
		adc_config.adc_hpf2_coef_B0 = 0;
		adc_config.adc_hpf2_coef_B1 = 0;
		adc_config.adc_hpf2_coef_A0 = 0;
		adc_config.adc_hpf2_coef_A1 = 0;
		adc_config.dtmf_wr_threshold = 8;
		adc_config.adcl_wr_threshold = 8;
		adc_config.dtmf_int_enable = AUD_DTMF_INT_DISABLE;
		adc_config.adcl_int_enable = AUD_ADCL_INT_DISABLE;
		adc_config.loop_adc2dac = AUD_LOOP_ADC2DAC_DISABLE;
		adc_config.agc_noise_thrd = 101;
		adc_config.agc_noise_high = 101;
		adc_config.agc_noise_low = 160;
		adc_config.agc_noise_min = 1;
		adc_config.agc_noise_tout = 0;
		adc_config.agc_high_dur = 3;
		adc_config.agc_low_dur = 3;
		adc_config.agc_min = 1;
		adc_config.agc_max = 4;
		adc_config.agc_ng_method = AUD_AGC_NG_METHOD_MUTE;
		adc_config.agc_ng_enable = AUD_AGC_NG_DISABLE;
		adc_config.agc_decay_time = AUD_AGC_DECAY_TIME_128;
		adc_config.agc_attack_time = AUD_AGC_ATTACK_TIME_128;
		adc_config.agc_high_thrd = 18;
		adc_config.agc_low_thrd = 0;
		adc_config.agc_iir_coef = AUD_AGC_IIR_COEF_1_1024;
		adc_config.agc_enable = AUD_AGC_DISABLE;
		adc_config.manual_pga_value = 0;
		adc_config.manual_pga_enable = AUD_GAC_MANUAL_PGA_DISABLE;
		adc_config.adc_fracmod_manual = AUD_ADC_TRACMOD_MANUAL_DISABLE;
		adc_config.adc_fracmod = 0;


		dac_config.dac_enable = AUD_DAC_DISABLE;
		dac_config.samp_rate = 1;
		dac_config.dac_hpf2_coef_B2 = 0x3A22;
		dac_config.dac_hpf2_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
		dac_config.dac_hpf1_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
		dac_config.dac_set_gain = 0x2D;
		dac_config.dac_clk_invert = AUD_DAC_CLK_INVERT_RISING;

		dac_config.dac_hpf2_coef_B0 = 0x3A22;
		dac_config.dac_hpf2_coef_B1 = 0x8BBF;

		dac_config.dac_hpf2_coef_A1 = 0x751C;
		dac_config.dac_hpf2_coef_A2 = 0xC9E6;

		dac_config.dacr_rd_threshold = 0x4;
		dac_config.dacl_rd_threshold = 0x4;
		dac_config.dacr_int_enable = 0x0;
		dac_config.dacl_int_enable = 0x0;
		
		dac_config.dac_filt_enable = AUD_DAC_FILT_DISABLE;
		dac_config.dac_fracmod_manual_enable = AUD_DAC_FRACMOD_MANUAL_DISABLE;
		dac_config.dac_fracmode_value = 0x0;

		//init audio driver
		ret = bk_aud_driver_init();
		if (ret != BK_OK)
			return;

		ret = bk_aud_adc_init(AUD_ADC_WORK_MODE_ADC, &adc_config, NULL);

		if (ret != BK_OK)
			return;

		bk_aud_dac_init(&dac_config);
		CLI_LOGI("init audio driver and adc successful\n");

		//register isr
		bk_aud_register_aud_isr(AUD_ISR_ADCL, cli_aud_adcl_isr, NULL);
		if (ret != BK_OK)
			return;
		CLI_LOGI("register adcl isr successful\n");

		//disable audio interrupt when loop test
		bk_aud_cpu_int_en(0);
		bk_aud_disable_adc_int();
		CLI_LOGI("disable adc interrupt successful\n");

		//start adc
		ret = bk_aud_start_adc();
		//start adc
		ret = bk_aud_start_dac();
		if (ret != BK_OK)
			return;
		//TODO
		//bk_aud_start_dac();
		CLI_LOGI("enable adc successful\n");
		//aud_struct_dump();

		//enable adc to dac loop test
		ret = bk_aud_start_loop_test();
		//aud_struct_dump();

		if (ret != BK_OK)
			return;
		CLI_LOGI("enable adc to dac loop test successful\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		CLI_LOGI("audio adc to dac loop test stop\n");

		//disable adc and dac
		ret = bk_aud_stop_adc();
		if (ret != BK_OK)
			return;
		//TODO
		//ret = bk_aud_stop_dac();
		if (ret != BK_OK)
			return;
		CLI_LOGI("audio adc to dac loop test stop successful\n");

		//stop loop test
		ret = bk_aud_stop_loop_test();
		if (ret != BK_OK)
			return;
		bk_aud_driver_deinit();
		CLI_LOGI("audio adc to dac loop test stop successful\n");
	} else {
		cli_aud_help();
		return;
	}
}

static void cli_aud_dtmf_loop_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_dtmf_config_t dtmf_config;
	aud_dac_config_t dac_config;

	if (argc != 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("audio dtmf loop test start\n");
		dtmf_config.adc_enable = AUD_ADC_DISABLE;
		dtmf_config.line_enable = AUD_ADC_LINE_DISABLE;
		dtmf_config.dtmf_enable = AUD_DTMF_DISABLE;
		dtmf_config.tone_pattern = AUD_DTMF_TONE_PATTERN_DTMF;
		dtmf_config.tone_mode = AUD_DTMF_TONE_MODE_CONTIUS;
		dtmf_config.tone_pause_time = 2;
		dtmf_config.tone_active_time = 4;
		dtmf_config.tone1_step = 10240;
		dtmf_config.tone1_attu = AUD_DTMF_TONE_ATTU_MINUS_1;
		dtmf_config.tone1_enable = AUD_DTMF_TONE_ENABLE;
		dtmf_config.tone2_step = 5734;
		dtmf_config.tone1_attu = AUD_DTMF_TONE_ATTU_MINUS_1;
		dtmf_config.tone1_enable = AUD_DTMF_TONE_ENABLE;
		dtmf_config.dtmf_wr_threshold = 8;
		dtmf_config.dtmf_int_enable = AUD_DTMF_INT_DISABLE;
		dtmf_config.loop_ton2dac = AUD_LOOP_DTMF2DAC_DISABLE;

		dac_config.dac_enable = AUD_DAC_DISABLE;
		dac_config.samp_rate = AUD_DAC_SAMP_RATE_8K;
		dac_config.dac_hpf2_coef_B2 = 0x3A22;
		dac_config.dac_hpf2_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
		dac_config.dac_hpf1_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
		dac_config.dac_set_gain = 0x2D;
		dac_config.dac_clk_invert = AUD_DAC_CLK_INVERT_RISING;

		dac_config.dac_hpf2_coef_B0 = 0x3A22;
		dac_config.dac_hpf2_coef_B1 = 0x8BBF;

		dac_config.dac_hpf2_coef_A1 = 0x751C;
		dac_config.dac_hpf2_coef_A2 = 0xC9E6;

		dac_config.dacr_rd_threshold = 0x4;
		dac_config.dacl_rd_threshold = 0x4;
		dac_config.dacr_int_enable = 0x0;
		dac_config.dacl_int_enable = 0x0;

		dac_config.dac_filt_enable = AUD_DAC_FILT_DISABLE;
		dac_config.dac_fracmod_manual_enable = AUD_DAC_FRACMOD_MANUAL_DISABLE;
		dac_config.dac_fracmode_value = 0x0;

		//init audio driver
		ret = bk_aud_driver_init();
		if (ret != BK_OK)
			return;
		ret = bk_aud_adc_init(AUD_ADC_WORK_MODE_DTMF, NULL, &dtmf_config);
		if (ret != BK_OK)
			return;

		bk_aud_dac_init(&dac_config);
		CLI_LOGI("init audio driver and dtmf successful\n");

		//register isr
		ret = bk_aud_register_aud_isr(AUD_ISR_DTMF, cli_aud_dtmf_isr, NULL);
		if (ret != BK_OK)
			return;
		CLI_LOGI("register dtmf isr successful\n");

		//disable audio interrupt when loop test
		bk_aud_cpu_int_en(0);
		bk_aud_disable_adc_int();
		CLI_LOGI("disable dtmf interrupt successful\n");

		//TODO
		//bk_aud_enable_dac_int();

		//start dtmf
		ret = bk_aud_start_adc();
		//start adc
		ret = bk_aud_start_dac();
		if (ret != BK_OK)
			return;
		CLI_LOGI("enable dtmf and dac successful\n");

		//enable dtmf to dac loop test
		ret = bk_aud_start_loop_test();
		if (ret != BK_OK)
			return;
		//aud_struct_dump();
		CLI_LOGI("enable dtmf to dac loop test successful\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		CLI_LOGI("audio dtmf loop test stop\n");
		//disable adc and dac
		ret = bk_aud_stop_adc();
		if (ret != BK_OK)
			return;
		//stop loop test
		ret = bk_aud_stop_loop_test();
		if (ret != BK_OK)
			return;
		bk_aud_driver_deinit();
		CLI_LOGI("audio dtmf loop test stop successful\n");
	} else {
		cli_aud_help();
		return;
	}
}

static void cli_aud_pcm_mcp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int i = 0;
	bk_err_t ret = BK_OK;
	uint32_t *aud_ptr = NULL;
	uint32_t aud_len = 0;
//	uint32_t *aud_addr = (uint32_t *)(0x47800000 + 0x33*4);
	aud_dac_config_t dac_config;
	aud_dac_status_t *dac_fifo_status = NULL;

	if (argc != 2) {
		cli_aud_help();
		return;
	}

	dac_config.dac_enable = AUD_DAC_DISABLE;
	dac_config.dac_hpf2_coef_B2 = 0x3A22;
	dac_config.dac_hpf2_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
	dac_config.dac_hpf1_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
	dac_config.dac_set_gain = 0x2D;
	dac_config.dac_clk_invert = AUD_DAC_CLK_INVERT_RISING;

	dac_config.dac_hpf2_coef_B0 = 0x3A22;
	dac_config.dac_hpf2_coef_B1 = 0x8BBF;

	dac_config.dac_hpf2_coef_A1 = 0x751C;
	dac_config.dac_hpf2_coef_A2 = 0xC9E6;

	dac_config.dacr_rd_threshold = 0x4;
	dac_config.dacl_rd_threshold = 0x4;
	dac_config.dacr_int_enable = 0x0;
	dac_config.dacl_int_enable = 0x0;

	dac_config.dac_filt_enable = AUD_DAC_FILT_DISABLE;
	dac_config.dac_fracmod_manual_enable = AUD_DAC_FRACMOD_MANUAL_DISABLE;
	dac_config.dac_fracmode_value = 0x0;

	if (os_strcmp(argv[1], "8000") == 0) {
		dac_config.samp_rate = AUD_DAC_SAMP_RATE_8K;
		aud_ptr = (uint32_t *)PCM_8000;
		aud_len = sizeof(PCM_8000) / sizeof(PCM_8000[0]);
		CLI_LOGI("audio pcm test 8000 start\n");
	} else if (os_strcmp(argv[1], "16000") == 0) {
		dac_config.samp_rate = 1;
		aud_ptr = (uint32_t *)PCM_16000;
		aud_len = sizeof(PCM_16000) / sizeof(PCM_16000[0]);
		CLI_LOGI("audio pcm test 16000 start\n");
	}else if (os_strcmp(argv[1], "44100") == 0) {
		dac_config.samp_rate = 2;
		aud_ptr = (uint32_t *)PCM_44100;
		aud_len = sizeof(PCM_44100) / sizeof(PCM_44100[0]);
		CLI_LOGI("audio pcm test 44100 start\n");
	}else if (os_strcmp(argv[1], "48000") == 0) {
		dac_config.samp_rate = 3;
		aud_ptr = (uint32_t *)PCM_48000;
		aud_len = sizeof(PCM_48000) / sizeof(PCM_48000[0]);
		CLI_LOGI("audio pcm test 48000 start\n");
	}else {
		CLI_LOGI("unsupport pcm test rate\n");
	}

	//init audio driver
	ret = bk_aud_driver_init();
	if (ret != BK_OK)
		return;
	ret = bk_aud_dac_init(&dac_config);
	if (ret != BK_OK)
		return;
	CLI_LOGI("init audio driver and dac successful\n");

	//start adc
	ret = bk_aud_start_dac();
	if (ret != BK_OK)
		return;

	CLI_LOGI("enable dac successful\n");

	ret = bk_aud_get_dac_status(dac_fifo_status);
	if (ret != BK_OK)
		return;
	CLI_LOGI("get dac status successful\n");

	while(1) {
		if(dac_fifo_status->dacl_near_empty) {
			bk_aud_dac_write(aud_ptr[i++]);
			if(i == aud_len -1) {
				i = 0;
			}
		}
	}

}

static void cli_aud_pcm_dma_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_dac_config_t dac_config;
	dma_config_t dma_config;
	uint32_t dac_fifo_addr;
	uint32_t *aud_ptr = NULL;
	uint32_t aud_len = 0;

	if (argc != 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "8000") == 0) {
		dac_config.samp_rate = AUD_DAC_SAMP_RATE_8K;
		aud_ptr = (uint32_t *)PCM_8000;
		aud_len = sizeof(PCM_8000) / sizeof(PCM_8000[0]);
		CLI_LOGI("audio pcm dma test 8000 start\n");
	} else if (os_strcmp(argv[1], "16000") == 0) {
		dac_config.samp_rate = 1;
		aud_ptr = (uint32_t *)PCM_16000;
		aud_len = sizeof(PCM_16000) / sizeof(PCM_16000[0]);
		CLI_LOGI("audio pcm dma test 16000 start\n");
	}else if (os_strcmp(argv[1], "44100") == 0) {
		dac_config.samp_rate = 2;
		aud_ptr = (uint32_t *)PCM_44100;
		aud_len = sizeof(PCM_44100) / sizeof(PCM_44100[0]);
		CLI_LOGI("audio pcm dma test 44100 start\n");
	}else if (os_strcmp(argv[1], "48000") == 0) {
		dac_config.samp_rate = 3;
		aud_ptr = (uint32_t *)PCM_48000;
		aud_len = sizeof(PCM_48000) / sizeof(PCM_48000[0]);
		CLI_LOGI("audio pcm dma test 48000 start\n");
	}else {
		CLI_LOGI("unsupport pcm dma test rate\n");
	}

	dac_config.dac_enable = AUD_DAC_DISABLE;

	dac_config.dac_hpf2_coef_B2 = 0x3A22;
	dac_config.dac_hpf2_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
	dac_config.dac_hpf1_bypass_enable = AUD_DAC_HPF_BYPASS_DISABLE;
	dac_config.dac_set_gain = 0x2D;
	dac_config.dac_clk_invert = AUD_DAC_CLK_INVERT_RISING;

	dac_config.dac_hpf2_coef_B0 = 0x3A22;
	dac_config.dac_hpf2_coef_B1 = 0x8BBF;

	dac_config.dac_hpf2_coef_A1 = 0x751C;
	dac_config.dac_hpf2_coef_A2 = 0xC9E6;

	dac_config.dacr_rd_threshold = 0x4;
	dac_config.dacl_rd_threshold = 0x4;
	dac_config.dacr_int_enable = 0x0;
	dac_config.dacl_int_enable = 0x0;

	dac_config.dac_filt_enable = AUD_DAC_FILT_DISABLE;
	dac_config.dac_fracmod_manual_enable = AUD_DAC_FRACMOD_MANUAL_DISABLE;
	dac_config.dac_fracmode_value = 0x0;

	//init audio driver
	ret = bk_aud_driver_init();
	if (ret != BK_OK)
		return;
	ret = bk_aud_dac_init(&dac_config);
	if (ret != BK_OK)
		return;
	CLI_LOGI("init audio driver and dac successful\n");

	//init dma driver
	ret = bk_dma_driver_init();
	if (ret != BK_OK) {
		CLI_LOGE("dma driver init failed\r\n");
		return;
	}
	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 1;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.dev = DMA_DEV_AUDIO;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;

	//get dac fifo address
	if (bk_aud_get_dac_fifo_addr(&dac_fifo_addr) != BK_OK) {
		CLI_LOGE("get dac fifo address failed\r\n");
		return;
	} else {
		dma_config.dst.start_addr = dac_fifo_addr;
		dma_config.dst.end_addr = dac_fifo_addr + 4;
	}

	dma_config.src.start_addr = (uint32_t)aud_ptr;
	dma_config.src.end_addr = (uint32_t)aud_ptr + aud_len;

	//os_printf("source_addr:0x%x, dest_addr:0x%x\r\n", dma_config.src.start_addr, dma_config.src.end_addr);

	//init dma channel
	ret = bk_dma_init(DMA_ID_2, &dma_config);
	if (ret != BK_OK) {
		CLI_LOGE("dma init failed\r\n");
		return;
	}

	//start adc
	ret = bk_aud_start_dac();

	bk_dma_set_transfer_len(DMA_ID_2, aud_len);
	ret = bk_dma_start(DMA_ID_2);
	if (ret != BK_OK)
		return;
	CLI_LOGI("audio pcm dma dac test successful\n");

	//aud_struct_dump();
}

static void cli_aud_adc_to_file_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_adc_config_t adc_config;
	dma_config_t dma_config;
	uint32_t adc_fifo_addr;
	uint32_t i = 0;

	if (argc != 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("audio adc to file test start\n");
		adc_test_mode = ADC_TEST_MODE_MCP;
		adc_config.samp_rate = AUD_ADC_SAMP_RATE_8K;
		adc_config.adc_enable = AUD_ADC_DISABLE;
		adc_config.line_enable = AUD_ADC_LINE_DISABLE;
		adc_config.dtmf_enable = AUD_DTMF_DISABLE;
		adc_config.adc_hpf2_coef_B2 = 0;
		adc_config.adc_hpf2_bypass_enable = AUD_ADC_HPF_BYPASS_ENABLE;
		adc_config.adc_hpf1_bypass_enable = AUD_ADC_HPF_BYPASS_ENABLE;
		adc_config.adc_set_gain = 0x2d;
		adc_config.adc_samp_edge = AUD_ADC_SAMP_EDGE_RISING;
		adc_config.adc_hpf2_coef_B0 = 0;
		adc_config.adc_hpf2_coef_B1 = 0;
		adc_config.adc_hpf2_coef_A0 = 0;
		adc_config.adc_hpf2_coef_A1 = 0;
		adc_config.dtmf_wr_threshold = 8;
		adc_config.adcl_wr_threshold = 8;
		adc_config.dtmf_int_enable = AUD_DTMF_INT_DISABLE;
		adc_config.adcl_int_enable = AUD_ADCL_INT_DISABLE;
		adc_config.loop_adc2dac = AUD_LOOP_ADC2DAC_DISABLE;
		adc_config.agc_noise_thrd = 101;
		adc_config.agc_noise_high = 101;
		adc_config.agc_noise_low = 160;
		adc_config.agc_noise_min = 1;
		adc_config.agc_noise_tout = 0;
		adc_config.agc_high_dur = 3;
		adc_config.agc_low_dur = 3;
		adc_config.agc_min = 1;
		adc_config.agc_max = 4;
		adc_config.agc_ng_method = AUD_AGC_NG_METHOD_MUTE;
		adc_config.agc_ng_enable = AUD_AGC_NG_DISABLE;
		adc_config.agc_decay_time = AUD_AGC_DECAY_TIME_128;
		adc_config.agc_attack_time = AUD_AGC_ATTACK_TIME_128;
		adc_config.agc_high_thrd = 18;
		adc_config.agc_low_thrd = 0;
		adc_config.agc_iir_coef = AUD_AGC_IIR_COEF_1_1024;
		adc_config.agc_enable = AUD_AGC_DISABLE;
		adc_config.manual_pga_value = 0;
		adc_config.manual_pga_enable = AUD_GAC_MANUAL_PGA_DISABLE;
		adc_config.adc_fracmod_manual = AUD_ADC_TRACMOD_MANUAL_DISABLE;
		adc_config.adc_fracmod = 0;


		//init audio driver
		bk_aud_driver_init();

		bk_aud_adc_init(AUD_ADC_WORK_MODE_ADC, &adc_config, NULL);
		CLI_LOGI("init audio driver, adc and dac successful\n");

		//init dma driver
		ret = bk_dma_driver_init();
		if (ret != BK_OK) {
			CLI_LOGE("dma driver init failed\r\n");
			return;
		}
		dma_config.mode = DMA_WORK_MODE_REPEAT;
		dma_config.chan_prio = 0;
		dma_config.src.dev = DMA_DEV_AUDIO;
		dma_config.src.width = DMA_DATA_WIDTH_32BITS;
		dma_config.dst.dev = DMA_DEV_DTCM;
		dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
		//get adc fifo address
		if (bk_aud_get_adc_fifo_addr(&adc_fifo_addr) != BK_OK) {
			CLI_LOGE("get adc fifo address failed\r\n");
			return;
		} else {
			dma_config.src.start_addr = adc_fifo_addr;
			dma_config.src.end_addr = adc_fifo_addr + 4;
		}
		dma_config.dst.start_addr = (uint32_t)&pcm_data[0];
		dma_config.dst.end_addr = (uint32_t)&pcm_data[test_size-1];

		//os_printf("source_addr:0x%x, dest_addr:0x%x\r\n", dma_config.src.start_addr, dma_config.dst.start_addr);

		//init dma channel
		ret = bk_dma_init(DMA_ID_2, &dma_config);
		if (ret != BK_OK) {
			CLI_LOGE("dma init failed\r\n");
			return;
		}
		//start dma
		bk_dma_set_transfer_len(DMA_ID_2, test_size * 4);
		bk_dma_start(DMA_ID_2);

		//start adc and dac
		bk_aud_start_adc();

		if (pcm_data[test_size - 1] != 0) {
			bk_aud_stop_adc();
			bk_dma_stop(DMA_ID_2);
		}
		CLI_LOGI("enable adc successful\n");

		//aud_struct_dump();

		for (i = 0; i<test_size; i++) {
			os_printf("%x", pcm_data[i]);
		}

		CLI_LOGI("start audio adc to file test successful\r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		CLI_LOGI("audio adc to file test stop\n");
		bk_aud_driver_deinit();
		bk_dma_deinit(DMA_ID_2);
		os_memset(pcm_data, 0, test_size);
		adc_test_mode = ADC_TEST_MODE_NULL;
		CLI_LOGI("audio adc to file test stop successful\n");
	} else {
		cli_aud_help();
		return;
	}
}


#define AUD_CMD_CNT (sizeof(s_aud_commands) / sizeof(struct cli_command))
static const struct cli_command s_aud_commands[] = {
	{"aud_adc_mcp_test", "aud_adc_mcp_test {start|stop}", cli_aud_adc_mcp_test_cmd},
	{"aud_dtmf_mcp_test", "aud_dtmf_mcp_test {start|stop}", cli_aud_dtmf_mcp_test_cmd},
	{"aud_adc_dma_test", "aud_adc_dma_test {start|stop}", cli_aud_adc_dma_test_cmd},
	{"aud_dtmf_dma_test", "aud_dtmf_dma_test {start|stop}", cli_aud_dtmf_dma_test_cmd},
	{"aud_adc_loop_test", "aud_adc_loop_test {start|stop}", cli_aud_adc_loop_test_cmd},
	{"aud_dtmf_loop_test", "aud_dtmf_loop_test {start|stop}", cli_aud_dtmf_loop_test_cmd},
	{"aud_pcm_mcp_test", "aud_pcm_mcp_test {8000|16000|44100|48000}", cli_aud_pcm_mcp_test_cmd},
	{"aud_pcm_dma_test", "aud_pcm_dma_test {8000|16000|44100|48000|stop}", cli_aud_pcm_dma_test_cmd},
	{"aud_adc_to_file_test", "aud_adc_to_file_test {start|stop}", cli_aud_adc_to_file_test_cmd},
};

int cli_aud_init(void)
{
	return cli_register_commands(s_aud_commands, AUD_CMD_CNT);
}

