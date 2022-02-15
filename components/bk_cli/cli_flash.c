#include <stdlib.h>
#include "cli.h"
#include "BkDriverFlash.h"

/*
format:  FLASH  E/R/W  0xABCD
example: FLASH  R  0x00100
*/

extern bk_err_t test_flash_write(volatile uint32_t start_addr, uint32_t len);
extern bk_err_t test_flash_erase(volatile uint32_t start_addr, uint32_t len);
extern bk_err_t test_flash_read(volatile uint32_t start_addr, uint32_t len);
extern bk_err_t test_flash_read_time(volatile uint32_t start_addr, uint32_t len);

static void flash_command_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char cmd = 0;
	uint32_t len = 0;
	uint32_t addr = 0;

	if (argc == 4) {
		cmd = argv[1][0];
		addr = atoi(argv[2]);
		len = atoi(argv[3]);

		switch (cmd) {
		case 'E':
			bk_flash_enable_security(FLASH_PROTECT_NONE);
			test_flash_erase(addr, len);
			bk_flash_enable_security(FLASH_UNPROTECT_LAST_BLOCK);
			break;

		case 'R':
			test_flash_read(addr, len);
			break;
		case 'W':
			bk_flash_enable_security(FLASH_PROTECT_NONE);
			test_flash_write(addr, len);
			bk_flash_enable_security(FLASH_UNPROTECT_LAST_BLOCK);
			break;
		//to check whether protection mechanism can work
		case 'N':
			test_flash_erase(addr, len);
			break;
		case 'M':
			test_flash_write(addr, len);
			break;
		case 'T':
			test_flash_read_time(addr, len);
			break;
		default:
			break;
		}
	} else
		os_printf("FLASH <R/W/E/M/N/T> <start_addr> <len>\r\n");
}


static void partShow_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_partition_t i;
	bk_logic_partition_t *partition;

	for (i = BK_PARTITION_BOOTLOADER; i <= BK_PARTITION_MAX; i++) {
		partition = bk_flash_get_info(i);
		if (partition == NULL)
			continue;

		os_printf("%4d | %11s |  Dev:%d  | 0x%08lx | 0x%08lx |\r\n", i,
				  partition->partition_description, partition->partition_owner,
				  partition->partition_start_addr, partition->partition_length);
	};

}

#define FLASH_CMD_CNT (sizeof(s_flash_commands) / sizeof(struct cli_command))
static const struct cli_command s_flash_commands[] = {
	{"fmap",    "flash memory map",      partShow_Command},
	{"flash",   "flash <cmd(R/W/E/N)>", flash_command_test},
};

int cli_flash_init(void)
{
	return cli_register_commands(s_flash_commands, FLASH_CMD_CNT);
}
