#include "video_transfer_save.h"

#if (CONFIG_SDCARD_HOST)
#include "ff.h"
#include "diskio.h"

static FIL file_ptr;
static uint8_t file_created = 0;
#endif

void f_create_file_to_sdcard(uint32_t frame_id)
{
#if (CONFIG_SDCARD_HOST)
	char *file_path = ".jpg";
	char cFileName[50];

	os_memset(&file_ptr, 0, sizeof(FIL));
	sprintf(cFileName, "%d:%d%s", DISK_NUMBER_SDIO_SD, frame_id, file_path);
	FRESULT fr = f_open(&file_ptr, cFileName, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK) {
		file_created = 0;
		os_printf("open %s fail.\r\n", cFileName);
		return;
	}
	file_created = 1;
#endif
}

void f_write_data_to_sdcard(void *data, uint32_t len, uint8_t is_eof)
{
#if (CONFIG_SDCARD_HOST)
	if (file_created == 0)
		return;
	unsigned int uiTemp = 0;
	FRESULT fr = f_write(&file_ptr, data, len, &uiTemp);
	if (fr != FR_OK) {
		os_printf("can not write file!\n");
		file_created = 0;
		f_close(&file_ptr);
		return;
	}

	if (is_eof) {
		fr = f_close(&file_ptr);
		file_created = 0;
		if (fr != FR_OK) {
			os_printf("can not close file!\n");
			return;
		}
	}
#endif
}

void f_close_write_to_sdcard(void)
{
#if (CONFIG_SDCARD_HOST)
	if (file_created == 0)
		return;
	f_close(&file_ptr);
	file_created = 0;
#endif
}

