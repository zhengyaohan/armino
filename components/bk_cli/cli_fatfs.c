#include "cli.h"

#if CONFIG_FATFS

#include "test_fatfs.h"

static void fatfs_operate(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t cmd;
	char *disk_name[DISK_NUMBER_COUNT] = {"ram", "sdio_sd", "udisk"};
	DISK_NUMBER drv_num = DISK_NUMBER_SDIO_SD;
	char file_name[64];
	char write_content[64];
	uint64_t content_len = 0;
	uint32_t test_cnt = 0;
	
	if (argc >= 3) {
		cmd = argv[1][0];
		drv_num = os_strtoul(argv[2], NULL, 10);
		if(argc >= 4)
		{
			snprintf(&file_name[0], sizeof(file_name) - 1, argv[3]);
			file_name[sizeof(file_name)-1] = 0;

			if(argc >= 5)
			{
				snprintf(&write_content[0], sizeof(write_content) - 1, argv[4]);
				write_content[sizeof(file_name)-1] = 0;

				if(argc >= 6)
				{
					content_len = os_strtoul(argv[5], NULL, 10);
					content_len = content_len > (sizeof(write_content) - 1) ? (sizeof(write_content) - 1) : content_len;
					content_len = content_len > strlen(write_content)? strlen(write_content) : content_len;

					//re-use in autotest
					test_cnt = os_strtoul(argv[5], NULL, 10);
				}
			}
		}
		switch (cmd) {
		case 'M':
			test_mount(drv_num);
			os_printf("mount:%s\r\n", disk_name[drv_num%DISK_NUMBER_COUNT]);
			break;
		case 'R':
			if (argc >= 5)
				content_len = os_strtoul(argv[4], NULL, 10);
			else
				content_len = 0x0fffffffffffffff;	//read finish
			test_fatfs_read(drv_num, file_name, content_len);
			os_printf("read %s, len_h = %d, len_l = %d\r\n", file_name, (uint32_t)(content_len>>32), (uint32_t)content_len);
			break;
		case 'W':
			test_fatfs_append_write(drv_num, file_name, write_content, content_len);
			os_printf("append and write:%s,%s\r\n", file_name, write_content);
			break;
		//fatfstest D dev-num file-name start-addr dump-len
		case 'D':
		{
			uint32_t start_addr = 0;
			if(argc >= 5)
			{
				start_addr = os_strtoul(argv[4], NULL, 10);
			}
			if(argc >= 6)
			{
				content_len = os_strtoul(argv[5], NULL, 10);
			}
			test_fatfs_dump(drv_num, file_name, start_addr, content_len);
			break;
		}

		//fatfstest A dev-num file-name write-len test_cnt
		//fatfstest A 1 autotest.txt 12487 3
		case 'A':
			if(argc >= 5)
			{
				content_len = os_strtoul(argv[4], NULL, 10);
			}
			test_fatfs_auto_test(drv_num, file_name, content_len, test_cnt);
			break;
		case 'F':
			test_fatfs_format(drv_num);
			os_printf("format :%x\r\n");
			break;
		case 'S':
			scan_file_system(drv_num);
			os_printf("scan \r\n");
			break;
		default:
			break;
		}
	} else
		os_printf("cmd param error\r\n");
}



#define FATFS_CMD_CNT (sizeof(s_fatfs_commands) / sizeof(struct cli_command))
static const struct cli_command s_fatfs_commands[] = {
	{"fatfstest", "fatfstest <cmd>", fatfs_operate},
};

int cli_fatfs_init(void)
{
	return cli_register_commands(s_fatfs_commands, FATFS_CMD_CNT);
}

#endif
