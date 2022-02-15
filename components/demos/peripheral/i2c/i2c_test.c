#ifndef _I2C_TEST_H_
#define _I2C_TEST_H_

#define I2C_TEST
#ifdef I2C_TEST
#include "include.h"
#include "arm_arch.h"
#include "string.h"
#include "stdlib.h"
#include "mem_pub.h"
#include "uart_pub.h"
#include "gpio_pub.h"
#include "gpio.h"
#include "rtos_pub.h"
#include <string.h>
#include "icu_pub.h"
#include "i2c_pub.h"
#include "bk_drv_model.h"

#define I2C_TEST_LEGNTH						32
#define I2C_TEST_EEPROM_LEGNTH				8
#define I2C_SLAVE_ADDRESS					0x50

#ifdef I2C_EEPROM_DEBUG
#define I2C_EEPROM_PRT      os_printf
#define I2C_EEPROM_WARN     warning_prf
#define I2C_EEPROM_FATAL    fatal_prf
#else
#define I2C_EEPROM_PRT      null_prf
#define I2C_EEPROM_WARN     null_prf
#define I2C_EEPROM_FATAL    os_printf
#endif

#if(!CONFIG_SOC_BK7271)

static unsigned long i2c1_write_eeprom(unsigned char op_addr, unsigned char *pData, unsigned char len)
{
	DD_HANDLE i2c_hdl;
	unsigned int status;
	unsigned int oflag;
	I2C_OP_ST i2c1_op;

	I2C_EEPROM_PRT("----- I2C1_write_eeprom start -----\r\n");

	oflag = 0;
	i2c_hdl = ddev_open(I2C1_DEV_NAME, &status, oflag);
	ASSERT(DD_HANDLE_UNVALID != i2c_hdl);

	i2c1_op.op_addr  = op_addr;
	i2c1_op.salve_id = I2C_SLAVE_ADDRESS;

	do {
		status = ddev_write(i2c_hdl, pData, len, (unsigned long)&i2c1_op);
	} while (status != 0);

	ddev_close(i2c_hdl);

	I2C_EEPROM_PRT("----- I2C1_write_eeprom over  -----\r\n");
	return 0;
}

static unsigned long i2c1_read_eeprom(unsigned char op_addr, unsigned char *pData, unsigned char len)
{
	unsigned char i;
	DD_HANDLE i2c_hdl;
	unsigned int status;
	unsigned int oflag;
	I2C_OP_ST i2c1_op;

	I2C_EEPROM_PRT("----- I2C1_read_eeprom start -----\r\n");

	oflag = 0;
	i2c_hdl = ddev_open(I2C1_DEV_NAME, &status, oflag);
	ASSERT(DD_HANDLE_UNVALID != i2c_hdl);

	i2c1_op.op_addr  = op_addr;
	i2c1_op.salve_id = I2C_SLAVE_ADDRESS;

	do {
		status = ddev_read(i2c_hdl, pData, len, (unsigned long)&i2c1_op);
	} while (status != 0);

	for (i = 0; i < 8; i++)
		I2C_EEPROM_PRT("pData[%d] = 0x%x\r\n", i, pData[i]);

	ddev_close(i2c_hdl);

	I2C_EEPROM_PRT("----- I2C1_read_eeprom over  -----\r\n");
	return status;
}

static void i2c1_test_eeprom_init_data(uint8_t *buffer, uint32_t len, uint32_t test_cnt)
{
	for (int i = 0; i < len; i++)
		buffer[i] = (i << 2) + 0x01 + test_cnt;
}

static void i2c1_test_eeprom_check_result(uint8_t *write_data, uint8_t *read_data,  uint32_t len)
{
	if (os_memcmp(write_data, read_data, len) == 0)
		os_printf("I2C1_test_eeprom: memcmp %d ok!\r\n", len);
	else {
		I2C_EEPROM_FATAL("I2C1_test_eeprom: memcmp  error!\r\n");

		for (int i = 0; i < 8; i++) {
			I2C_EEPROM_FATAL("read_data[%d]=0x%x, read_data[%d]=0x%x\r\n",
							 i, write_data[i], i, read_data[i]);
		}
	}
}

static unsigned long i2c1_test_eeprom_write_read(uint32_t start_addr, uint32_t len, uint32_t test_cnt)
{
	uint8_t read_data[I2C_TEST_EEPROM_LEGNTH] = {0};
	uint8_t write_data[I2C_TEST_EEPROM_LEGNTH] = {0};

	i2c1_test_eeprom_init_data(write_data, len, test_cnt);
	i2c1_write_eeprom(start_addr, write_data, len);
	delay_ms(100);
	i2c1_read_eeprom(start_addr, read_data, len);
	i2c1_test_eeprom_check_result(write_data, read_data, len);

	return 0;
}


static unsigned long i2c1_test_eeprom(void)
{
	uint32_t start_addr = 0;

	I2C_EEPROM_PRT("----- i2c1_test_eeprom start -----\r\n");

	for (int i = 0; i < 100; i++) {
		start_addr = i * I2C_TEST_EEPROM_LEGNTH;
		i2c1_test_eeprom_write_read(start_addr, I2C_TEST_EEPROM_LEGNTH, i);
		delay_ms(100);
	}

	I2C_EEPROM_PRT("----- i2c1_test_eeprom over  -----\r\n");
	return 0;
}
#endif

static uint32 i2c2_test(int argc, char **argv)
{
	int i, j, ret, test_length;
	DD_HANDLE i2c_hdl;
	unsigned int status;
	unsigned int oflag;
	I2C_OP_ST i2c2_op;
	I2C2_MSG_ST i2c_msg_config;

	bk_printf(" i2c2_test start  \r\n");

	if (argc != 3)
		return 1;

	test_length = atoi(argv[2]);

	i2c_msg_config.pData = (UINT8 *)os_malloc(test_length);
	if (i2c_msg_config.pData == NULL) {
		bk_printf("malloc fail\r\n");
		goto exit;
	}

	if (os_strcmp(argv[1], "master_wr") == 0) {
		bk_printf("i2c2 master write\r\n");

		for (i = 0; i < test_length; i++)
			i2c_msg_config.pData[i] = (i << 2) + 0x01 ;

		oflag =   0 & (~I2C2_MSG_WORK_MODE_MS_BIT)   // master
				  & (~I2C2_MSG_WORK_MODE_AL_BIT)   // 7bit address
				  & (~I2C2_MSG_WORK_MODE_IA_BIT);  // without inner address

		i2c_hdl = ddev_open(I2C2_DEV_NAME, &status, oflag);
		ASSERT(DD_HANDLE_UNVALID != i2c_hdl);

		i2c2_op.op_addr    = 0x00;
		i2c2_op.salve_id   = 0x72;		//only master : send slave address
		i2c2_op.slave_addr = 0x73;		//only slave  : config slave address

		do {
			status = ddev_write(i2c_hdl, i2c_msg_config.pData, test_length, (unsigned long)&i2c2_op);
		} while (status != 0);
	}
	if (os_strcmp(argv[1], "master_rd") == 0) {
		bk_printf("i2c2 master read\r\n");

		oflag =   0 & (~I2C2_MSG_WORK_MODE_MS_BIT)   // master
				  & (~I2C2_MSG_WORK_MODE_AL_BIT)   // 7bit address
				  & (~I2C2_MSG_WORK_MODE_IA_BIT);  // without inner address
		i2c_hdl = ddev_open(I2C2_DEV_NAME, &status, oflag);
		ASSERT(DD_HANDLE_UNVALID != i2c_hdl);

		i2c2_op.op_addr    = 0x00;
		i2c2_op.salve_id   = 0x72;		//only master : send slave address
		i2c2_op.slave_addr = 0x73;		//only slave  : config slave address

		do {
			status = ddev_read(i2c_hdl, i2c_msg_config.pData, test_length, (unsigned long)&i2c2_op);
		} while (status != 0);

	}
	if (os_strcmp(argv[1], "slave_wr") == 0) {

		bk_printf("i2c2 slave write\r\n");

		for (i = 0; i < test_length; i++)
			i2c_msg_config.pData[i] = (i << 2) + 0x01 ;

		oflag = (0 | (I2C2_MSG_WORK_MODE_MS_BIT))     // slave
				& (~I2C2_MSG_WORK_MODE_AL_BIT)   // 7bit address
				& (~I2C2_MSG_WORK_MODE_IA_BIT);  // without inner address
		i2c_hdl = ddev_open(I2C2_DEV_NAME, &status, oflag);
		ASSERT(DD_HANDLE_UNVALID != i2c_hdl);

		i2c2_op.op_addr    = 0x00;
		i2c2_op.salve_id   = 0x73;		//only master : send slave address
		i2c2_op.slave_addr = 0x72;		//only slave  : config slave addresss
		i2c2_op.mode	   = i2c_msg_config.WkMode;

		do {
			status = ddev_write(i2c_hdl, i2c_msg_config.pData, test_length, (unsigned long)&i2c2_op);
		} while (status != 0);
	}
	if (os_strcmp(argv[1], "slave_rd") == 0) {

		bk_printf("i2c2 slave read\r\n");

		oflag = (0 | (I2C2_MSG_WORK_MODE_MS_BIT))     // slave
				& (~I2C2_MSG_WORK_MODE_AL_BIT)   // 7bit address
				& (~I2C2_MSG_WORK_MODE_IA_BIT);  // without inner address
		i2c_hdl = ddev_open(I2C2_DEV_NAME, &status, oflag);
		ASSERT(DD_HANDLE_UNVALID != i2c_hdl);

		i2c2_op.op_addr    = 0x00;
		i2c2_op.salve_id   = 0x73;		//only master : send slave address
		i2c2_op.slave_addr = 0x72;		//only slave  : config slave addresss
		i2c2_op.mode	   = i2c_msg_config.WkMode;

		do {
			status = ddev_read(i2c_hdl, i2c_msg_config.pData, test_length, (unsigned long)&i2c2_op);
		} while (status != 0);
	}

	for (i = 0; i < test_length; i++)
		bk_printf("pData[%d]=0x%x\r\n", i, i2c_msg_config.pData[i]);

	ddev_close(i2c_hdl);

	bk_printf(" i2c2 test over\r\n");
	return 0;

exit:

	if (NULL != i2c_msg_config.pData) {
		os_free(i2c_msg_config.pData);
		i2c_msg_config.pData = NULL;
	}
}

static uint32 i2c2_test_eeprom(int argc, char **argv)
{
	int i, j, ret;
	DD_HANDLE i2c_hdl;
	unsigned int status;
	unsigned int oflag;
	I2C_OP_ST i2c2_op;
	I2C2_MSG_ST i2c_msg_config;

	bk_printf(" i2c2_test_eeprom start  \r\n");

	if (argc != 2)
		return 1;

	i2c_msg_config.pData = (UINT8 *)os_malloc(I2C_TEST_EEPROM_LEGNTH);
	if (i2c_msg_config.pData == NULL) {
		bk_printf("malloc fail\r\n");
		goto exit;
	}

	oflag	= (0 & (~I2C2_MSG_WORK_MODE_MS_BIT)				// master
			   & (~I2C2_MSG_WORK_MODE_AL_BIT)) 				// 7bit address
			  | (I2C2_MSG_WORK_MODE_IA_BIT);				// with inner address

	i2c_hdl = ddev_open(I2C2_DEV_NAME, &status, oflag);
	ASSERT(DD_HANDLE_UNVALID != i2c_hdl);

	if (os_strcmp(argv[1], "write_eeprom") == 0) {
		bk_printf("eeprom write\r\n");

		for (i = 0; i < I2C_TEST_EEPROM_LEGNTH; i++)
			i2c_msg_config.pData[i] = (i << 2) + 0x10 ;

		i2c2_op.op_addr    = 0x08;
		i2c2_op.salve_id   = 0x50;		//send slave address
		i2c2_op.slave_addr = 0x73;		//slave: as slave address

		do {
			status = ddev_write(i2c_hdl, i2c_msg_config.pData, I2C_TEST_EEPROM_LEGNTH, (unsigned long)&i2c2_op);
		} while (status != 0);
	}
	if (os_strcmp(argv[1], "read_eeprom") == 0) {
		bk_printf("eeprom read\r\n");

		i2c2_op.op_addr    = 0x08;
		i2c2_op.salve_id   = 0x50;		//send slave address
		i2c2_op.slave_addr = 0x73;		//slave: as slave address

		do {
			status = ddev_read(i2c_hdl, i2c_msg_config.pData, I2C_TEST_EEPROM_LEGNTH, (unsigned long)&i2c2_op);
		} while (status != 0);
	}

	for (i = 0; i < I2C_TEST_EEPROM_LEGNTH; i++)
		bk_printf("pData[%d]=0x%x\r\n", i, i2c_msg_config.pData[i]);

	ddev_close(i2c_hdl);

	bk_printf(" i2c2 test over\r\n");
	return 0;

exit:

	if (NULL != i2c_msg_config.pData) {
		os_free(i2c_msg_config.pData);
		i2c_msg_config.pData = NULL;
	}
}

#endif
#endif







