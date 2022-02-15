#include "include.h"
#include <stdlib.h>

#include "stdarg.h"
#include "bk_api_mem.h"
#include "sys_rtos.h"
#include "bk_api_rtos.h"
#include "bk_kernel_err.h"
#include "bk_sys_ctrl.h"
#include "bk_cli.h"
#include "BkDriverPwm.h"
#if CONFIG_LWIP
#include "lwip/ping.h"
#endif
#if CONFIG_BLE
#include "bk_ble.h"
#endif
//#include "sensor.h"
#include "spi_pub.h"
#include "i2c_pub.h"
#include "BkDriverPwm.h"
#include "bk_drv_model.h"
#include "bk_log.h"
#include "cli.h"
#include "BkDriverGpio.h"

#define TAG "peri_test"

#if CONFIG_PERI_TEST

INT32 os_strcmp(const char *s1, const char *s2);


#define I2C_TEST_LEGNTH					32
#define I2C_TEST_EEPROM_LEGNTH			8

#if CONFIG_I2C1_TEST
static void i2c1_test_eeprom(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{

	int i;
	DD_HANDLE i2c_hdl;
	unsigned int status;
	unsigned int oflag;
	I2C_OP_ST i2c1_op;
	I2C1_MSG_ST i2c_msg_config;

	CLI_LOGI(" i2c1_test_eeprom start  \r\n");

	i2c_msg_config.pData = (UINT8 *)os_malloc(I2C_TEST_EEPROM_LEGNTH);
	if (i2c_msg_config.pData == NULL) {
		CLI_LOGI("malloc fail\r\n");
		goto exit;
	}

	oflag   = (0 & (~I2C1_MSG_WORK_MODE_MS_BIT)     // master
			   & (~I2C1_MSG_WORK_MODE_AL_BIT))    // 7bit address
			  | (I2C1_MSG_WORK_MODE_IA_BIT);     // with inner address

	i2c_hdl = ddev_open(DD_DEV_TYPE_I2C1, &status, oflag);

	if (os_strcmp(argv[1], "write_eeprom") == 0) {
		CLI_LOGI("eeprom write\r\n");

		for (i = 0; i < I2C_TEST_EEPROM_LEGNTH; i++)
			i2c_msg_config.pData[i] = (i << 2) + 0x10 ;

		i2c1_op.op_addr    = 0x08;
		i2c1_op.salve_id   = 0x50;      //send slave address
		i2c1_op.slave_addr = 0x73;      //slave: as slave address

		do {
			status = ddev_write(i2c_hdl, (char *)i2c_msg_config.pData, I2C_TEST_EEPROM_LEGNTH, (unsigned long)&i2c1_op);
		} while (status != 0);
	}
	if (os_strcmp(argv[1], "read_eeprom") == 0) {
		CLI_LOGI("eeprom read\r\n");

		i2c1_op.op_addr    = 0x08;
		i2c1_op.salve_id   = 0x50;      //send slave address
		i2c1_op.slave_addr = 0x73;      //slave: as slave address

		do {
			status = ddev_read(i2c_hdl, (char *)i2c_msg_config.pData, I2C_TEST_EEPROM_LEGNTH, (unsigned long)&i2c1_op);
		} while (status != 0);
	}

	for (i = 0; i < I2C_TEST_EEPROM_LEGNTH; i++)
		CLI_LOGI("pData[%d]=0x%x\r\n", i, i2c_msg_config.pData[i]);

	ddev_close(i2c_hdl);

	CLI_LOGI(" i2c2 test over\r\n");

exit:

	if (NULL != i2c_msg_config.pData) {
		os_free(i2c_msg_config.pData);
		i2c_msg_config.pData = NULL;
	}
}
#endif

#if CONFIG_I2C2_TEST
static void i2c2_test_eeprom(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{

	int i;
	DD_HANDLE i2c_hdl;
	unsigned int status;
	unsigned int oflag;
	I2C_OP_ST i2c2_op;
	I2C2_MSG_ST i2c_msg_config;

	CLI_LOGI(" i2c2_test_eeprom start  \r\n");

	i2c_msg_config.pData = (UINT8 *)os_malloc(I2C_TEST_EEPROM_LEGNTH);
	if (i2c_msg_config.pData == NULL) {
		CLI_LOGI("malloc fail\r\n");
		goto exit;
	}

	oflag   = (0 & (~I2C2_MSG_WORK_MODE_MS_BIT)     // master
			   & (~I2C2_MSG_WORK_MODE_AL_BIT))    // 7bit address
			  | (I2C2_MSG_WORK_MODE_IA_BIT);     // with inner address

	i2c_hdl = ddev_open(DD_DEV_TYPE_I2C2, &status, oflag);

	if (os_strcmp(argv[1], "write_eeprom") == 0) {
		CLI_LOGI("eeprom write\r\n");

		for (i = 0; i < I2C_TEST_EEPROM_LEGNTH; i++)
			i2c_msg_config.pData[i] = (i << 2) + 0x10 ;

		i2c2_op.op_addr     = 0x08;
		i2c2_op.salve_id    = 0x50;     //send slave address
		i2c2_op.slave_addr  = 0x73;     //slave: as slave address

		do {
			status = ddev_write(i2c_hdl, (char *)i2c_msg_config.pData, I2C_TEST_EEPROM_LEGNTH, (unsigned long)&i2c2_op);
		} while (status != 0);
	}
	if (os_strcmp(argv[1], "read_eeprom") == 0) {
		CLI_LOGI("eeprom read\r\n");

		i2c2_op.op_addr    = 0x08;
		i2c2_op.salve_id   = 0x50;      //send slave address
		i2c2_op.slave_addr = 0x73;      //slave: as slave address

		do {
			status = ddev_read(i2c_hdl, (char *)i2c_msg_config.pData, I2C_TEST_EEPROM_LEGNTH, (unsigned long)&i2c2_op);
		} while (status != 0);
	}

	for (i = 0; i < I2C_TEST_EEPROM_LEGNTH; i++)
		CLI_LOGI("pData[%d]=0x%x\r\n", i, i2c_msg_config.pData[i]);
	os_free(i2c_msg_config.pData);

	ddev_close(i2c_hdl);
	CLI_LOGI(" i2c2 test over\r\n");

exit:

	if (NULL != i2c_msg_config.pData) {
		os_free(i2c_msg_config.pData);
		i2c_msg_config.pData = NULL;
	}
}
#endif

#if CONFIG_SPI_TEST
#define SPI_BAUDRATE       (26 * 1000 * 1000)
#define SPI_BAUDRATE_5M    (5* 1000 * 1000)

#define SPI_TX_BUF_LEN     (1024)
#define SPI_RX_BUF_LEN     (1024)

#if (CONFIG_SOC_BK7271)
int spi_channel;
#endif

void gspi_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	struct spi_message  msg;
	UINT32 max_hz;
	UINT32 mode;

#if (CONFIG_SOC_BK7271)
	spi_channel = 2;
#endif

	/* SPI Interface with Clock Speeds Up to 30 MHz */
	if (argc == 5)
	{
		max_hz = SPI_BAUDRATE ;//atoi(argv[3]);
	} else
	{
		max_hz = SPI_BAUDRATE; //master/slave
	}

	if (os_strcmp(argv[1], "master") == 0)
	{
		mode = SPI_MODE_0 | SPI_MSB | SPI_MASTER;

		//bk_spi_master_init (cfg->max_hz, cfg->mode);
	} else if (os_strcmp(argv[1], "slave") == 0)
	{
		mode = SPI_MODE_0 | SPI_MSB | SPI_SLAVE;

		bk_spi_slave_init(max_hz, mode);
	}
#if CONFIG_SPI_DMA
	else if (os_strcmp(argv[1], "slave_dma_rx") == 0)
	{
		UINT8 *buf;
		int rx_len, ret;

		if (argc < 2)
			rx_len = SPI_RX_BUF_LEN;
		else
			rx_len = atoi(argv[2]);

		CLI_LOGI("spi dma rx: rx_len:%d\n", rx_len);

		buf = os_malloc(rx_len * sizeof(UINT8));
		if (!buf) {
			CLI_LOGW("spi test malloc buf fail\r\n");
			return ;
		}

		os_memset(buf, 0, rx_len);

		msg.send_buf = NULL;
		msg.send_len = 0;
		msg.recv_buf = buf;
		msg.recv_len = rx_len;

		mode = SPI_MODE_0 | SPI_MSB | SPI_SLAVE;
		max_hz = atoi(argv[3]);

		bk_spi_dma_init(mode, max_hz, &msg);

		ret = bk_spi_dma_transfer(mode, &msg);
		if (ret)
			CLI_LOGW("spi dma recv error%d\r\n", ret);
		else {
			for (int i = 0; i < rx_len; i++) {
				bk_printf("%02x,", buf[i]);
				if ((i + 1) % 32 == 0)
					bk_printf("\r\n");
			}
			bk_printf("\r\n");
			os_free(buf);
		}
	} else if ((os_strcmp(argv[1], "slave_dma_tx") == 0))
	{
		UINT8 *buf;
		int tx_len, ret;

		if (argc < 2)
			tx_len = SPI_RX_BUF_LEN;
		else
			tx_len = atoi(argv[2]);

		CLI_LOGI("spi dma tx: tx_len:%d,%d\n", tx_len, max_hz);

		buf = os_malloc(tx_len * sizeof(UINT8));
		if (!buf) {
			CLI_LOGW("spi test malloc buf fail\r\n");
			return ;
		}

		os_memset(buf, 0, tx_len);

		for (int i = 0; i < tx_len; i++)
			buf[i] = i & 0xFF;

		msg.send_buf = buf;
		msg.send_len = tx_len;
		msg.recv_buf = NULL;
		msg.recv_len = 0;

		mode = SPI_MODE_0 | SPI_MSB | SPI_SLAVE;
		max_hz = atoi(argv[3]);

		bk_spi_dma_init(mode, max_hz, &msg);

		ret = bk_spi_dma_transfer(mode, &msg);
		if (ret)
			CLI_LOGW("spi dma send error%d\r\n", ret);
		else {
			for (int i = 0; i < tx_len; i++) {
				bk_printf("%02x,", buf[i]);
				if ((i + 1) % 32 == 0)
					bk_printf("\r\n");
			}
			bk_printf("\r\n");
			os_free(buf);
		}
	} else if ((os_strcmp(argv[1], "master_dma_tx") == 0))
	{
		UINT8 *buf;
		int tx_len, ret;

		if (argc < 2)
			tx_len = SPI_RX_BUF_LEN;
		else
			tx_len = atoi(argv[2]);

		max_hz = atoi(argv[3]);//SPI_BAUDRATE;

		CLI_LOGI("spi master  dma tx: tx_len:%d max_hz:%d\r\n", tx_len, max_hz);
#if (SPI_LINE_MODE == SPI_USE_3_LINE)
		BkGpioInitialize(15, OUTPUT_NORMAL);
		BkGpioInitialize(6, OUTPUT_NORMAL);
		BkGpioOutputLow(15);
		BkGpioOutputLow(6);
#endif

		buf = os_malloc(tx_len * sizeof(UINT8));
		if (!buf) {
			CLI_LOGW("spi test malloc buf fail\r\n");
			return ;
		}

		os_memset(buf, 0, tx_len);

		for (int i = 0; i < tx_len; i++)
			buf[i] = i & 0xFF;

		msg.send_buf = buf;
		msg.send_len = tx_len;
		msg.recv_buf = NULL;
		msg.recv_len = 0;

		mode = SPI_MODE_0 | SPI_MSB | SPI_MASTER;

		bk_spi_dma_init(mode, max_hz, &msg);

		ret = bk_spi_dma_transfer(mode,&msg);
		if (ret)
			CLI_LOGW("spi dma send error%d\r\n", ret);
		else {
#if (SPI_LINE_MODE == SPI_USE_3_LINE)
			BkGpioOutputHigh(15);
			BkGpioOutputHigh(6);
#endif
			for (int i = 0; i < tx_len; i++) {
				bk_printf("%02x,", buf[i]);
				if ((i + 1) % 32 == 0)
					bk_printf("\r\n");
			}
			bk_printf("\r\n");
			os_free(buf);
		}
	} else if ((os_strcmp(argv[1], "master_dma_rx") == 0))
	{
		UINT8 *buf;
		int rx_len, ret;

		if (argc < 2)
			rx_len = SPI_RX_BUF_LEN;
		else
			rx_len = atoi(argv[2]) + 1;	//slave tx first send 0x72 so must send one more

		max_hz = atoi(argv[3]);//SPI_BAUDRATE;

		CLI_LOGI("spi master  dma rx: rx_len:%d max_hz:%d\r\n\n", rx_len, max_hz);

		buf = os_malloc(rx_len * sizeof(UINT8));
		if (!buf) {
			CLI_LOGW("spi test malloc buf fail\r\n");
			return ;
		}

		os_memset(buf, 0, rx_len);

		msg.send_buf = NULL;
		msg.send_len = 0;
		msg.recv_buf = buf;
		msg.recv_len = rx_len;

		mode = SPI_MODE_0 | SPI_MSB | SPI_MASTER;

		bk_spi_dma_init(mode, max_hz, &msg);

		ret = bk_spi_dma_transfer(mode,&msg);
		if (ret)
			CLI_LOGW("spi dma recv error%d\r\n", ret);
		else {
			for (int i = 1; i < rx_len; i++) {
				bk_printf("%02x,", buf[i]);
				if ((i + 1) % 32 == 0)
					bk_printf("\r\n");
			}
			bk_printf("\r\n");
			os_free(buf);
		}

	} else if ((os_strcmp(argv[1], "master_tx_loop") == 0))
	{
		UINT8 *buf;
		int tx_len, ret;
		UINT32 cnt = 0;

		if (argc < 2)
			tx_len = SPI_RX_BUF_LEN;
		else
			tx_len = atoi(argv[2]);

		max_hz = atoi(argv[3]); //SPI_BAUDRATE;

		CLI_LOGI("spi master  dma tx: tx_len:%d max_hz:%d\r\n", tx_len, max_hz);

		buf = os_malloc(tx_len * sizeof(UINT8));
		if (!buf) {
			CLI_LOGI("buf malloc fail\r\n");
			return;
		}

		os_memset(buf, 0, tx_len);

		for (int i = 0; i < tx_len; i++)
			buf[i] = i + 0x60;

		msg.send_buf = buf;
		msg.send_len = tx_len;
		msg.recv_buf = NULL;
		msg.recv_len = 0;

		mode = SPI_MODE_0 | SPI_MSB | SPI_MASTER;

		bk_spi_dma_init(mode, max_hz, &msg);

		while (1) {
			if (cnt >= 0x1000)
				break;

			ret = bk_spi_dma_transfer(mode,&msg);
			if (ret)
				CLI_LOGI("spi dma send error%d\r\n", ret);

			else
				bk_printf("%d\r\n", cnt++);
			rtos_delay_milliseconds(80);
		}
	}else if ((os_strcmp(argv[1], "stop") == 0))
	{
		mode = SPI_MODE_0 | SPI_MSB | SPI_MASTER;
		bk_spi_dma_deinit(mode);

		mode = SPI_SLAVE;
		bk_spi_dma_deinit(mode);
	}


#endif

	else
		CLI_LOGI("gspi_test master/slave	 tx/rx	rate  len\r\n");

	//CLI_LOGI("cfg:%d, 0x%02x, %d\r\n", cfg->data_width, cfg->mode, cfg->max_hz);

	if (os_strcmp(argv[2], "tx") == 0)
	{
		UINT8 *buf;
		int tx_len;

		if (argc < 4)
			tx_len = SPI_TX_BUF_LEN;
		else
			tx_len = atoi(argv[4]);

		CLI_LOGI("spi init tx_len:%d\n", tx_len);

		buf = os_malloc(tx_len * sizeof(UINT8));

		if (buf) {
			os_memset(buf, 0, tx_len);
			for (int i = 0; i < tx_len; i++)
				buf[i] = i & 0xff;
			msg.send_buf = buf;
			msg.send_len = tx_len;
			msg.recv_buf = NULL;
			msg.recv_len = 0;

			bk_spi_slave_xfer(&msg);

			for (int i = 0; i < tx_len; i++) {
				CLI_LOGI("%02x,", buf[i]);
				if ((i + 1) % 32 == 0)
					CLI_LOGI("\r\n");
			}
			CLI_LOGI("\r\n");

			os_free(buf);
		}
	} else if (os_strcmp(argv[2], "rx") == 0)
	{
		UINT8 *buf;
		int rx_len;

		if (argc < 4)
			rx_len = SPI_RX_BUF_LEN;
		else
			rx_len = atoi(argv[4]);

		CLI_LOGI("SPI_RX: rx_len:%d\n", rx_len);

		buf = os_malloc(rx_len * sizeof(UINT8));

		if (buf) {
			os_memset(buf, 0, rx_len);

			msg.send_buf = NULL;
			msg.send_len = 0;
			msg.recv_buf = buf;
			msg.recv_len = rx_len;

			//CLI_LOGI("buf:%d\r\n", buf);
			rx_len = bk_spi_slave_xfer(&msg);
			CLI_LOGI("rx_len:%d\r\n", rx_len);

			for (int i = 0; i < rx_len; i++) {
				CLI_LOGI("%02x,", buf[i]);
				if ((i + 1) % 32 == 0)
					CLI_LOGI("\r\n");
			}
			CLI_LOGI("\r\n");

			os_free(buf);
		}
	} else
	{
		//CLI_LOGI("gspi_test master/slave tx/rx rate len\r\n");
	}
}





uint32 spi_dma_slave_rx_thread_main(void);
uint32 spi_dma_master_tx_thread_main(void);
uint32 spi_dma_master_tx_loop_stop(void);
uint32 spi_dma_slave_rx_loop_stop(void);

void spi_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{

#if (CONFIG_SOC_BK7271)
	spi_channel = 2;
#endif

	if (os_strcmp(argv[1], "slave_rx_loop") == 0)
	{
		CLI_LOGI("spi dma rx loop test\r\n");
		spi_dma_slave_rx_thread_main();
	}
	else if (os_strcmp(argv[1], "master_tx_loop") == 0){

		CLI_LOGI("spi dma tx loop test\r\n");
		spi_dma_master_tx_thread_main();
	}
	else if (os_strcmp(argv[1], "master_tx_stop") == 0){

		CLI_LOGI("spi dma tx stop loop\r\n");
		spi_dma_master_tx_loop_stop();
	}else if (os_strcmp(argv[1], "slave_rx_stop") == 0){

		CLI_LOGI("spi dma slave rx stop loop\r\n");
		spi_dma_slave_rx_loop_stop();
	}
}


#endif


const struct cli_command peripheral_clis[] = {

#if CONFIG_SPI_TEST
	{"gspi_test", "general spi", gspi_test},

	{"spi_test", "spi dma rx loop", spi_Command},
#endif

#if CONFIG_I2C1_TEST
	{"i2c1_test", "i2c1_test write/read_eeprom", i2c1_test_eeprom},
#endif

#if CONFIG_I2C2_TEST
	{"i2c2_test", "i2c2_test write/read_eeprom", i2c2_test_eeprom},
#endif

};

int cli_peri_init(void)
{
	int ret;

	CLI_LOGI("peripheral cli int \r\n");
	ret = cli_register_commands(peripheral_clis, sizeof(peripheral_clis) / sizeof(struct cli_command));
	if (ret)
		CLI_LOGI("ret: %d peripheral commands fail.\r\n", ret);
	return BK_OK;
}

#endif
