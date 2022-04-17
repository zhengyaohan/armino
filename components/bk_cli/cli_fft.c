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

#include <os/os.h>
#include "cli.h"
#include <driver/fft.h>
#include <driver/fft_types.h>
#include "fft_driver.h"
#include "sys_driver.h"

#define FFT_TEST_DATA_SIZE      256

int16 data_proc_i[FFT_TEST_DATA_SIZE] = {0};
int16 data_proc_q[FFT_TEST_DATA_SIZE] = {0};

uint32 data_source[FFT_TEST_DATA_SIZE] = {
	0x7fff0000, 0x7c760000, 0x720c0000, 0x61540000, 0x4b3c0000, 0x30fb0000, 0x14060000, 0xf5f50000, 
	0xd8720000, 0xbd1f0000, 0xa57e0000, 0x92de0000, 0x86450000, 0x80660000, 0x81940000, 0x89bf0000, 
	0x98730000, 0xace00000, 0xc5e40000, 0xe21f0000, 0x00000000, 0x1de10000, 0x3a1c0000, 0x53200000, 
	0x678d0000, 0x76410000, 0x7e6c0000, 0x7f9a0000, 0x79bb0000, 0x6d220000, 0x5a820000, 0x42e10000, 
	0x278e0000, 0x0a0b0000, 0xebfa0000, 0xcf050000, 0xb4c40000, 0x9eac0000, 0x8df40000, 0x838a0000, 
	0x80010000, 0x838a0000, 0x8df40000, 0x9eac0000, 0xb4c40000, 0xcf050000, 0xebfa0000, 0x0a0b0000, 
	0x278e0000, 0x42e10000, 0x5a820000, 0x6d220000, 0x79bb0000, 0x7f9a0000, 0x7e6c0000, 0x76410000, 
	0x678d0000, 0x53200000, 0x3a1c0000, 0x1de10000, 0x00000000, 0xe21f0000, 0xc5e40000, 0xace00000, 
	0x98730000, 0x89bf0000, 0x81940000, 0x80660000, 0x86450000, 0x92de0000, 0xa57e0000, 0xbd1f0000, 
	0xd8720000, 0xf5f50000, 0x14060000, 0x30fb0000, 0x4b3c0000, 0x61540000, 0x720c0000, 0x7c760000, 
	0x7fff0000, 0x7c760000, 0x720c0000, 0x61540000, 0x4b3c0000, 0x30fb0000, 0x14060000, 0xf5f50000, 
	0xd8720000, 0xbd1f0000, 0xa57e0000, 0x92de0000, 0x86450000, 0x80660000, 0x81940000, 0x89bf0000, 
	0x98730000, 0xace00000, 0xc5e40000, 0xe21f0000, 0x00000000, 0x1de10000, 0x3a1c0000, 0x53200000, 
	0x678d0000, 0x76410000, 0x7e6c0000, 0x7f9a0000, 0x79bb0000, 0x6d220000, 0x5a820000, 0x42e10000, 
	0x278e0000, 0x0a0b0000, 0xebfa0000, 0xcf050000, 0xb4c40000, 0x9eac0000, 0x8df40000, 0x838a0000, 
	0x80010000, 0x838a0000, 0x8df40000, 0x9eac0000, 0xb4c40000, 0xcf050000, 0xebfa0000, 0x0a0b0000, 
	0x278e0000, 0x42e10000, 0x5a820000, 0x6d220000, 0x79bb0000, 0x7f9a0000, 0x7e6c0000, 0x76410000, 
	0x678d0000, 0x53200000, 0x3a1c0000, 0x1de10000, 0x00000000, 0xe21f0000, 0xc5e40000, 0xace00000, 
	0x98730000, 0x89bf0000, 0x81940000, 0x80660000, 0x86450000, 0x92de0000, 0xa57e0000, 0xbd1f0000, 
	0xd8720000, 0xf5f50000, 0x14060000, 0x30fb0000, 0x4b3c0000, 0x61540000, 0x720c0000, 0x7c760000, 
	0x7fff0000, 0x7c760000, 0x720c0000, 0x61540000, 0x4b3c0000, 0x30fb0000, 0x14060000, 0xf5f50000, 
	0xd8720000, 0xbd1f0000, 0xa57e0000, 0x92de0000, 0x86450000, 0x80660000, 0x81940000, 0x89bf0000, 
	0x98730000, 0xace00000, 0xc5e40000, 0xe21f0000, 0x00000000, 0x1de10000, 0x3a1c0000, 0x53200000, 
	0x678d0000, 0x76410000, 0x7e6c0000, 0x7f9a0000, 0x79bb0000, 0x6d220000, 0x5a820000, 0x42e10000, 
	0x278e0000, 0x0a0b0000, 0xebfa0000, 0xcf050000, 0xb4c40000, 0x9eac0000, 0x8df40000, 0x838a0000, 
	0x80010000, 0x838a0000, 0x8df40000, 0x9eac0000, 0xb4c40000, 0xcf050000, 0xebfa0000, 0x0a0b0000, 
	0x278e0000, 0x42e10000, 0x5a820000, 0x6d220000, 0x79bb0000, 0x7f9a0000, 0x7e6c0000, 0x76410000, 
	0x678d0000, 0x53200000, 0x3a1c0000, 0x1de10000, 0x00000000, 0xe21f0000, 0xc5e40000, 0xace00000, 
	0x98730000, 0x89bf0000, 0x81940000, 0x80660000, 0x86450000, 0x92de0000, 0xa57e0000, 0xbd1f0000, 
	0xd8720000, 0xf5f50000, 0x14060000, 0x30fb0000, 0x4b3c0000, 0x61540000, 0x720c0000, 0x7c760000, 
	0x7fff0000, 0x7c760000, 0x720c0000, 0x61540000, 0x4b3c0000, 0x30fb0000, 0x14060000, 0xf5f50000, 
	0xd8720000, 0xbd1f0000, 0xa57e0000, 0x92de0000, 0x86450000, 0x80660000, 0x81940000, 0x89bf0000
} ;

uint32 data_proc[FFT_TEST_DATA_SIZE] = {
	0xfe6b0000, 0xfe6400cf, 0xfe4e01ad, 0xfe2602ab, 0xfde503e2, 0xfd7e0580, 0xfcd207e5, 0xfb960bfa, 
	0xf8c014f7, 0xec503b78, 0x1e3ca207, 0x08e2e3ed, 0x0559eefb, 0x03e8f394, 0x0321f61d, 0x02a5f7bb, 
	0x0251f8dc, 0x0215f9b2, 0x01e8fa58, 0x01c5fadc, 0x01a9fb49, 0x0193fba4, 0x0180fbf1, 0x0171fc34, 
	0x0164fc6f, 0x0158fca2, 0x014ffcd0, 0x0146fcf9, 0x013ffd1f, 0x0139fd41, 0x0133fd60, 0x012efd7c, 
	0x0129fd96, 0x0125fdae, 0x0121fdc5, 0x011efdda, 0x011bfdee, 0x0118fe00, 0x0116fe12, 0x0113fe22, 
	0x0111fe32, 0x010ffe40, 0x010dfe4e, 0x010bfe5b, 0x010afe68, 0x0109fe74, 0x0107fe7f, 0x0106fe8a, 
	0x0105fe95, 0x0104fe9f, 0x0103fea8, 0x0102feb1, 0x0101feba, 0x0100fec3, 0x00fffecb, 0x00fefed4, 
	0x00fefedb, 0x00fdfee3, 0x00fcfeea, 0x00fcfef1, 0x00fbfef8, 0x00fbfefe, 0x00faff05, 0x00faff0b, 
	0x00f9ff11, 0x00f9ff17, 0x00f8ff1e, 0x00f8ff23, 0x00f8ff29, 0x00f7ff2e, 0x00f7ff35, 0x00f6ff37, 
	0x00f6ff3d, 0x00f6ff42, 0x00f6ff46, 0x00f5ff4b, 0x00f5ff50, 0x00f5ff55, 0x00f5ff59, 0x00f4ff5e, 
	0x00f4ff62, 0x00f4ff66, 0x00f4ff6a, 0x00f4ff6e, 0x00f3ff72, 0x00f3ff76, 0x00f3ff7a, 0x00f3ff7d, 
	0x00f3ff81, 0x00f3ff85, 0x00f3ff88, 0x00f2ff8c, 0x00f2ff90, 0x00f2ff94, 0x00f2ff97, 0x00f2ff9b, 
	0x00f2ff9e, 0x00f1ffa2, 0x00f1ffa5, 0x00f1ffa8, 0x00f1ffab, 0x00f1ffaf, 0x00f1ffb1, 0x00f1ffb6, 
	0x00f1ffb8, 0x00f1ffbb, 0x00f1ffbe, 0x00f1ffc1, 0x00f1ffc5, 0x00f0ffc8, 0x00f0ffcb, 0x00f1ffce, 
	0x00f0ffd1, 0x00f0ffd4, 0x00f0ffd7, 0x00f0ffd9, 0x00f0ffdd, 0x00f0ffe0, 0x00f0ffe3, 0x00f0ffe5, 
	0x00f0ffe9, 0x00f0ffeb, 0x00f0ffef, 0x00f0fff1, 0x00f0fff4, 0x00f0fff7, 0x00f0fffa, 0x00f0fffc, 
	0x00f00000, 0x00f00002, 0x00f00005, 0x00f00008, 0x00f0000b, 0x00f0000e, 0x00f00010, 0x00f00014, 
	0x00f00016, 0x00f10019, 0x00f0001e, 0x00f00020, 0x00f00022, 0x00f00025, 0x00f00029, 0x00f0002b, 
	0x00f0002e, 0x00f00031, 0x00f00034, 0x00f10037, 0x00f1003a, 0x00f1003d, 0x00f10040, 0x00f10043, 
	0x00f10046, 0x00f1004a, 0x00f1004d, 0x00f10051, 0x00f10054, 0x00f10058, 0x00f2005b, 0x00f2005e, 
	0x00f20061, 0x00f20065, 0x00f20067, 0x00f2006c, 0x00f2006f, 0x00f20072, 0x00f30077, 0x00f3007a, 
	0x00f2007e, 0x00f20083, 0x00f30084, 0x00f30089, 0x00f3008d, 0x00f40091, 0x00f40095, 0x00f40099, 
	0x00f4009e, 0x00f400a2, 0x00f500a5, 0x00f500ab, 0x00f500af, 0x00f500b4, 0x00f600b9, 0x00f600bd, 
	0x00f600c2, 0x00f600c7, 0x00f700cb, 0x00f700d1, 0x00f700d7, 0x00f800dc, 0x00f800e2, 0x00f900e8, 
	0x00f900ee, 0x00fa00f4, 0x00fa00fa, 0x00fb0100, 0x00fb0107, 0x00fc010e, 0x00fc0115, 0x00fd011c, 
	0x00fe0124, 0x00ff012c, 0x00ff0134, 0x0100013c, 0x01010145, 0x0102014e, 0x01030157, 0x01040160, 
	0x0105016a, 0x01060174, 0x01070180, 0x0109018b, 0x010a0197, 0x010b01a4, 0x010d01b0, 0x010f01bf, 
	0x011101ce, 0x011301dd, 0x011601ed, 0x011801fe, 0x011b0211, 0x011e0226, 0x0121023a, 0x01250251, 
	0x01290269, 0x012e0283, 0x0133029f, 0x013902bf, 0x013f02e0, 0x01470305, 0x014f032e, 0x0159035d, 
	0x01640390, 0x017103ca, 0x0180040f, 0x0193045c, 0x01a904b6, 0x01c50522, 0x01e805a7, 0x0215064c, 
	0x02510723, 0x02a50844, 0x032109e2, 0x03e80c6b, 0x05591103, 0x08e31c11, 0x1e3c5df6, 0xec51c488, 
	0xf8c0eb08, 0xfb97f405, 0xfcd2f81b, 0xfd7efa7e, 0xfde6fc1d, 0xfe26fd54, 0xfe4ffe52, 0xfe64ff30
} ;

uint32 data_comp[] = {
	0x00ffffff, 0x00f8ffff, 0x00e4ffff, 0x00c2ffff, 0x0096ffff, 0x0061ffff, 0x0028ffff, 0xffebffff, 
	0xffb00000, 0xff7affff, 0xff4bffff, 0xff25ffff, 0xff0cffff, 0xff000000, 0xff03ffff, 0xff130000, 
	0xff300000, 0xff59ffff, 0xff8b0000, 0xffc4ffff, 0xffffffff, 0x003bffff, 0x0074ffff, 0x00a60000, 
	0x00cfffff, 0x00ecffff, 0x00fc0000, 0x00ffffff, 0x00f30000, 0x00daffff, 0x00b4ffff, 0x0085ffff, 
	0x004fffff, 0x00140000, 0xffd7ffff, 0xff9e0000, 0xff690000, 0xff3d0000, 0xff1b0000, 0xff070000, 
	0xff00ffff, 0xff07ffff, 0xff1bffff, 0xff3dffff, 0xff690000, 0xff9d0000, 0xffd7ffff, 0x00140000, 
	0x004fffff, 0x0085ffff, 0x00b50000, 0x00daffff, 0x00f30000, 0x00ffffff, 0x00fcffff, 0x00ecffff, 
	0x00cfffff, 0x00a60000, 0x00740000, 0x003b0000, 0xffff0000, 0xffc40000, 0xff8bffff, 0xff590000, 
	0xff300000, 0xff130000, 0xff03ffff, 0xff00ffff, 0xff0cffff, 0xff25ffff, 0xff4a0000, 0xff7affff, 
	0xffb00000, 0xffeb0000, 0x00280000, 0x0061ffff, 0x0096ffff, 0x00c20000, 0x00e4ffff, 0x00f8ffff, 
	0x00ff0000, 0x00f80000, 0x00e30000, 0x00c20000, 0x00960000, 0x00610000, 0x00280000, 0xffeb0000, 
	0xffb00000, 0xff7a0000, 0xff4bffff, 0xff250000, 0xff0c0000, 0xff00ffff, 0xff030000, 0xff13ffff, 
	0xff300000, 0xff590000, 0xff8bffff, 0xffc4ffff, 0xffff0000, 0x003bffff, 0x00740000, 0x00a6ffff, 
	0x00cfffff, 0x00ecffff, 0x00fc0000, 0x00ff0000, 0x00f3ffff, 0x00da0000, 0x00b40000, 0x00850000, 
	0x004f0000, 0x00140000, 0xffd70000, 0xff9e0000, 0xff690000, 0xff3d0000, 0xff1bffff, 0xff070000, 
	0xff000000, 0xff070000, 0xff1bffff, 0xff3dffff, 0xff690000, 0xff9effff, 0xffd70000, 0x0014ffff, 
	0x004f0000, 0x00850000, 0x00b50000, 0x00daffff, 0x00f30000, 0x00ffffff, 0x00fcffff, 0x00ec0000, 
	0x00cfffff, 0x00a60000, 0x0074ffff, 0x003b0000, 0xffff0000, 0xffc40000, 0xff8b0000, 0xff590000, 
	0xff30ffff, 0xff13ffff, 0xff03ffff, 0xff00ffff, 0xff0cffff, 0xff25ffff, 0xff4a0000, 0xff7affff, 
	0xffb0ffff, 0xffebffff, 0x00280000, 0x0061ffff, 0x0096ffff, 0x00c2ffff, 0x00e3ffff, 0x00f80000, 
	0x00ff0000, 0x00f8ffff, 0x00e40000, 0x00c2ffff, 0x0096ffff, 0x0061ffff, 0x00280000, 0xffeb0000, 
	0xffb00000, 0xff7a0000, 0xff4b0000, 0xff250000, 0xff0c0000, 0xff000000, 0xff030000, 0xff130000, 
	0xff300000, 0xff59ffff, 0xff8b0000, 0xffc4ffff, 0x0000ffff, 0x003bffff, 0x00740000, 0x00a6ffff, 
	0x00cf0000, 0x00ec0000, 0x00fc0000, 0x00ffffff, 0x00f3ffff, 0x00daffff, 0x00b4ffff, 0x0085ffff, 
	0x004f0000, 0x00140000, 0xffd7ffff, 0xff9d0000, 0xff69ffff, 0xff3d0000, 0xff1b0000, 0xff070000, 
	0xff00ffff, 0xff07ffff, 0xff1bffff, 0xff3dffff, 0xff690000, 0xff9effff, 0xffd70000, 0x0014ffff, 
	0x004f0000, 0x0085ffff, 0x00b4ffff, 0x00da0000, 0x00f30000, 0x00ff0000, 0x00fcffff, 0x00ec0000, 
	0x00cfffff, 0x00a60000, 0x00740000, 0x003b0000, 0xffff0000, 0xffc40000, 0xff8b0000, 0xff590000, 
	0xff300000, 0xff130000, 0xff03ffff, 0xff000000, 0xff0cffff, 0xff25ffff, 0xff4b0000, 0xff7affff, 
	0xffb00000, 0xffeb0000, 0x0028ffff, 0x0061ffff, 0x0096ffff, 0x00c2ffff, 0x00e3ffff, 0x00f8ffff, 
	0x00ff0000, 0x00f80000, 0x00e40000, 0x00c20000, 0x0096ffff, 0x00610000, 0x00280000, 0xffeb0000, 
	0xffb00000, 0xff7a0000, 0xff4a0000, 0xff250000, 0xff0c0000, 0xff000000, 0xff03ffff, 0xff13ffff,
} ;

extern void delay(int num);//TODO fix me

static void cli_fft_help(void)
{
	CLI_LOGI("fft_fft_test {start|stop}\r\n");
	CLI_LOGI("fft_ifft_test {start|stop}\r\n");
	CLI_LOGI("fft_fir_signal_test {start|stop} \r\n");
	CLI_LOGI("fft_fir_dual_test {start|stop} \r\n");
}

static void cli_fft_fft_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	uint32_t i = 0;
	int32 temp_q = 0, temp_i = 0;
	int32 comp_q = 0, comp_i = 0;
	fft_input_t fft_conf = {0};

	if (argc != 2) {
		cli_fft_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("fft test start\n");

		//init fft driver
		ret = bk_fft_driver_init();
		if (ret != BK_OK)
			return;
		CLI_LOGI("fft driver successful\r\n");

		fft_conf.inbuf = os_malloc(4*FFT_TEST_DATA_SIZE);
		os_memset(fft_conf.inbuf, 0, 4*FFT_TEST_DATA_SIZE);

		fft_conf.mode = FFT_WORK_MODE_FFT;
		fft_conf.size = FFT_TEST_DATA_SIZE;
		os_printf("\r\ndata_source:\n");
		for (i = 0; i < FFT_TEST_DATA_SIZE; i++) {
			fft_conf.inbuf[i] = data_source[i];
			//os_printf("0x%08x, ", fft_conf.inbuf[i]);
		}
		os_printf("\r\n");
		//start fft
		bk_fft_enable(&fft_conf);
		CLI_LOGI("start fft process \r\n");
		//fft_struct_dump();

		//wait fft complete
		while(bk_fft_is_busy())
			;
		CLI_LOGI("fft complete\r\n");

		os_free(fft_conf.inbuf);

		//read output data
		bk_fft_output_read(data_proc_i, data_proc_q, 2 * FFT_TEST_DATA_SIZE);
		os_printf("\r\ndata_proc:\n");
		for (i = 0; i < FFT_TEST_DATA_SIZE; i++) {
			//os_printf("0x%04hx%04hx, ", data_proc_q[i], data_proc_i[i]);
			temp_q = ((int32)data_proc_q[i]) & 0x0000ffff;
			temp_i = ((int32)data_proc_i[i]) & 0x0000ffff;
			comp_q = (data_proc[i] >> 16) & 0x0000ffff;
			comp_i = data_proc[i] & 0x0000ffff;
			os_printf("\r\ntemp_q:0x%8x, temp_i:0x%8x, comp_q:0x%8x, comp_i:0x%8x\r\n", temp_q, temp_i, comp_q, comp_i);
			if ((temp_q != comp_q) || (temp_i != comp_i)) {
				os_printf("\r\nfft test fail!\r\n");
				ret = BK_FAIL;
				break;
			}
		}

		if (ret == BK_OK)
			CLI_LOGI("start fft test successful\r\n");
		else
			CLI_LOGE("start fft test failed\r\n");

	} else if (os_strcmp(argv[1], "stop") == 0) {
		CLI_LOGI("fft test stop\n");
		bk_fft_driver_deinit();
		CLI_LOGI("fft test stop successful\n");
	} else {
		cli_fft_help();
		return;
	}
}

static void cli_fft_ifft_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	uint32_t i = 0;
	int32 temp_q = 0, temp_i = 0;
	int32 comp_q = 0, comp_i = 0;
	fft_input_t fft_conf = {0};

	if (argc != 2) {
		cli_fft_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("ifft test start\n");

		//init fft driver
		ret = bk_fft_driver_init();
		if (ret != BK_OK)
			return;
		CLI_LOGI("ifft driver successful\r\n");

		fft_conf.inbuf = os_malloc(4*FFT_TEST_DATA_SIZE);
		os_memset(fft_conf.inbuf, 0, 4*FFT_TEST_DATA_SIZE);

		fft_conf.mode = FFT_WORK_MODE_IFFT;
		fft_conf.size = FFT_TEST_DATA_SIZE;
		os_printf("\r\ndata_source:\n");
		for (i = 0; i < FFT_TEST_DATA_SIZE; i++) {
			fft_conf.inbuf[i] = data_proc[i];
			//os_printf("0x%08x, ", fft_conf.inbuf[i]);
		}
		os_printf("\r\n");
		//start fft
		bk_fft_enable(&fft_conf);
		CLI_LOGI("start ifft process \r\n");

		//wait fft complete
		while(bk_fft_is_busy())
			;
		CLI_LOGI("ifft complete\r\n");

		os_free(fft_conf.inbuf);

		//read output data
		bk_fft_output_read(data_proc_i, data_proc_q, 2 * FFT_TEST_DATA_SIZE);
		os_printf("\r\ndata_proc:\n");
		for (i = 0; i < FFT_TEST_DATA_SIZE; i++) {
			//os_printf("0x%04hx%04hx, ", data_proc_q[i], data_proc_i[i]);
			temp_q = ((int32)data_proc_q[i]) & 0x0000ffff;
			temp_i = ((int32)data_proc_i[i]) & 0x0000ffff;
			comp_q = (data_comp[i] >> 16) & 0x0000ffff;
			comp_i = data_comp[i] & 0x0000ffff;
			//os_printf("\r\ntemp_q:0x%8x, temp_i:0x%8x, comp_q:0x%8x, comp_i:0x%8x\r\n", temp_q, temp_i, comp_q, comp_i);
			if ((temp_q != comp_q) || (temp_i != comp_i)) {
				os_printf("\r\nifft test fail!\r\n");
				ret = BK_FAIL;
				break;
			}
		}

		if (ret == BK_OK)
			CLI_LOGI("start ifft test successful\r\n");
		else
			CLI_LOGE("start ifft test failed\r\n");

	} else if (os_strcmp(argv[1], "stop") == 0) {
		CLI_LOGI("ifft test stop\n");
		bk_fft_driver_deinit();
		CLI_LOGI("ifft test stop successful\n");
	} else {
		cli_fft_help();
		return;
	}
}

//fir function is not used
#if 0
static void cli_fft_fir_signal_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	uint32_t i = 0;
	fft_fir_input_t fir_conf = {0};

	if (argc != 2) {
		cli_fft_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("fir test start\n");

		//init fft driver
		ret = bk_fft_driver_init();
		if (ret != BK_OK)
			return;
		CLI_LOGI("fft driver successful\r\n");

		fir_conf.coef_c0 = os_malloc(2*FFT_TEST_DATA_SIZE);
		os_memset(fir_conf.coef_c0, 0, 2*FFT_TEST_DATA_SIZE);
		fir_conf.coef_c1 = NULL;
		fir_conf.input_d0 = os_malloc(2*FFT_TEST_DATA_SIZE);
		//os_memcpy(fir_conf.input_d0, data_source, 2*FFT_TEST_DATA_SIZE);
		os_memset(fir_conf.input_d0, 0, 2*FFT_TEST_DATA_SIZE);
		fir_conf.input_d1 = NULL;
		fir_conf.mode = FFT_FIR_MODE_SIGNAL;
		fir_conf.fir_len = FFT_TEST_DATA_SIZE;
		os_printf("\r\ndata_source:\n");
		for (i = 0; i < FFT_TEST_DATA_SIZE; i++)
			os_printf("%04x, ", fir_conf.input_d0[i]);
		os_printf("\r\n");

		//start fir
		bk_fft_fir_single_enable(&fir_conf);
		CLI_LOGI("start fir process \r\n");

		//wait fir complete
		while(bk_fft_is_busy())
			;
		CLI_LOGI("fft complete\r\n");

		os_free(fir_conf.coef_c0);
		os_free(fir_conf.input_d0);

		//read output data
		bk_fft_output_read(data_proc_i, data_proc_q, 2 * FFT_TEST_DATA_SIZE);
		os_printf("\r\ndata_proc:\n");
		for (i = 0; i < FFT_TEST_DATA_SIZE; i++)
			os_printf("%04x%04x, ", data_proc_q[i], data_proc_i[i]);

		if (ret == BK_OK)
			CLI_LOGI("start fir test successful\r\n");
		else
			CLI_LOGE("start fir test failed\r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		CLI_LOGI("fir test stop\n");
		bk_fft_driver_deinit();
		CLI_LOGI("fir test stop successful\n");
	} else {
		cli_fft_help();
		return;
	}
}

static void cli_fft_fir_dual_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	uint32_t i = 0;
	fft_fir_input_t fir_conf = {0};

	if (argc != 2) {
		cli_fft_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("fir test start\n");

		//init fft driver
		ret = bk_fft_driver_init();
		if (ret != BK_OK)
			return;
		CLI_LOGI("fft driver successful\r\n");

		fir_conf.coef_c0 = os_malloc(2*FFT_TEST_DATA_SIZE);
		os_memset(fir_conf.coef_c0, 0, 2*FFT_TEST_DATA_SIZE);
		fir_conf.coef_c1 = os_malloc(2*FFT_TEST_DATA_SIZE);
		os_memset(fir_conf.coef_c1, 0, 2*FFT_TEST_DATA_SIZE);
		fir_conf.input_d0 = os_malloc(2*FFT_TEST_DATA_SIZE);
		//os_memcpy(fir_conf.input_d0, data_source, 2*FFT_TEST_DATA_SIZE);
		os_memset(fir_conf.input_d0, 0, 2*FFT_TEST_DATA_SIZE);
		fir_conf.input_d1 = os_malloc(2*FFT_TEST_DATA_SIZE);
		os_memset(fir_conf.input_d1, 0, 2*FFT_TEST_DATA_SIZE);
		fir_conf.mode = FFT_FIR_MODE_SIGNAL;
		fir_conf.fir_len = FFT_TEST_DATA_SIZE;
		os_printf("\r\ndata_source:\n");
		for (i = 0; i < FFT_TEST_DATA_SIZE; i++)
			os_printf("%04x, ", fir_conf.input_d0[i]);
		os_printf("\r\n");

		//start fir
		bk_fft_fir_single_enable(&fir_conf);
		CLI_LOGI("start fir process \r\n");
//		fft_struct_dump();

		//wait fir complete
		while(bk_fft_is_busy())
			;
		CLI_LOGI("fft complete\r\n");

		//free memory
		os_free(fir_conf.coef_c0);
		os_free(fir_conf.coef_c1);
		os_free(fir_conf.input_d0);
		os_free(fir_conf.input_d1);

		//read output data
		bk_fft_output_read(data_proc_i, data_proc_q, 2 * FFT_TEST_DATA_SIZE);
		os_printf("\r\ndata_proc:\n");
		for (i = 0; i < FFT_TEST_DATA_SIZE; i++)
			os_printf("%04x%04x, ", data_proc_q[i], data_proc_i[i]);

		if (ret == BK_OK)
			CLI_LOGI("start fir test successful\r\n");
		else
			CLI_LOGE("start fir test failed\r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		CLI_LOGI("fir test stop\n");
		bk_fft_driver_deinit();
		CLI_LOGI("fir test stop successful\n");
	} else {
		cli_fft_help();
		return;
	}
}
#endif

#define FFT_CMD_CNT (sizeof(s_fft_commands) / sizeof(struct cli_command))
static const struct cli_command s_fft_commands[] = {
	{"fft_fft_test", "fft_fft_test {start|stop}", cli_fft_fft_test_cmd},
	{"fft_ifft_test", "fft_ifft_test {start|stop}", cli_fft_ifft_test_cmd},
	//{"fft_fir_signal_test", "fft_fir_signal_test {start|stop}", cli_fft_fir_signal_test_cmd},
	//{"fft_fir_dual_test", "fft_fir_dual_test {start|stop}", cli_fft_fir_dual_test_cmd},
};

int cli_fft_init(void)
{
	return cli_register_commands(s_fft_commands, FFT_CMD_CNT);
}

