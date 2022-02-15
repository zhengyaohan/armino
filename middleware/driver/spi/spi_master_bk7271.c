#include "include.h"
#include "bk_arm_arch.h"
#include "bk_typedef.h"
#include "bk_arm_arch.h"
#include "sys_config.h"

#if(CONFIG_SOC_BK7271)
#include "bk_icu.h"
#include "spi_pub.h"
#include "bk_sys_ctrl.h"
#include "bk_drv_model.h"
#include "bk_api_mem.h"
#include "bk_kernel_err.h"
#include "bk_api_rtos.h"
#include "general_dma_pub.h"
#include "general_dma.h"
#include "spi_bk7271.h"


#if CONFIG_SPI_MASTER
struct bk_spi_dev {
	UINT8 *tx_ptr;
	UINT32 tx_len;
	beken_semaphore_t tx_sem;
	beken_semaphore_t rx_sem;

	UINT8 *rx_ptr;
	UINT32 rx_len;
	UINT32 rx_offset;
	UINT32 rx_drop;

	UINT32 total_len;
	UINT32 flag;

	beken_mutex_t mutex;
};

static struct bk_spi_dev *spi_dev;
extern int spi_channel;

static void bk_spi_rx_callback(int is_rx_end, void *param)
{
	UINT8 ch, *rxbuf;
	UINT32 offset, drop;

	GLOBAL_INT_DECLARATION();

	rxbuf = spi_dev->rx_ptr;
	drop = spi_dev->rx_drop;
	offset = spi_dev->rx_offset;

	while (1) {
		if (spi_read_rxfifo(&ch) == 0)
			break;

		if (rxbuf) {
			if (drop != 0)
				drop--;
			else {
				if (offset < spi_dev->rx_len) {
					rxbuf[offset] = ch;
					offset++;
				} else
					BK_SPI_WPRT("rx over flow:%02x, %d\r\n", ch, spi_dev->rx_len);
			}
		}
	}

	GLOBAL_INT_DISABLE();
	spi_dev->rx_drop = drop;
	spi_dev->rx_offset = offset;
	GLOBAL_INT_RESTORE();
}

static void bk_spi_tx_needwrite_callback(int port, void *param)
{
	UINT8 *tx_ptr = spi_dev->tx_ptr, data;
	UINT32 tx_len = spi_dev->tx_len, total_len = spi_dev->total_len, tx_ok = 0;

	UINT8 *rxbuf;
	UINT32 offset, drop;

	rxbuf = spi_dev->rx_ptr;
	drop = spi_dev->rx_drop;
	offset = spi_dev->rx_offset;

	GLOBAL_INT_DECLARATION();
	while (total_len) {
		tx_ok = 0;

		if (tx_len) {
			data = *tx_ptr;
			if (spi_write_txfifo(data) == 1) {
				tx_ok = 1;

				tx_len --;
				tx_ptr ++;
			}
		} else {
			data = 0xff;
			if (spi_write_txfifo(data) == 1)
				tx_ok = 1;
		}

		/* check rx data to prevent rx over flow */
		if (spi_read_rxfifo(&data) == 1) {
			if (rxbuf) {
				if (drop != 0)
					drop--;
				else {
					if (offset < spi_dev->rx_len) {
						rxbuf[offset] = data;
						offset++;
					} else
						BK_SPI_WPRT("0 rx over flow:%02x, %d\r\n", data, spi_dev->rx_len);
				}
			}
		}

		if (tx_ok == 1) {
			total_len --;
			if (total_len == 0) {
				UINT32 enable = 0;
				sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TXINT_EN, (void *)&enable);

				//BK_SPI_PRT("tx fin\r\n");
				break;
			}
		} else
			break;
	}

	GLOBAL_INT_DISABLE();
	spi_dev->tx_ptr = tx_ptr;
	spi_dev->tx_len = tx_len;
	spi_dev->total_len = total_len;

	spi_dev->rx_drop = drop;
	spi_dev->rx_offset = offset;
	GLOBAL_INT_RESTORE();

}

static void bk_spi_tx_finish_callback(int port, void *param)
{
	if ((spi_dev->total_len == 0) && ((spi_dev->flag & TX_FINISH_FLAG) == 0)) {
		spi_dev->flag |= TX_FINISH_FLAG;
		rtos_set_semaphore(&spi_dev->tx_sem);
		//BK_SPI_PRT("tx end\r\n");
	}
}

static void bk_spi_configure(UINT32 rate, UINT32 mode)
{
	UINT32 param;
	struct spi_callback_des spi_dev_cb;

	/* data bit width */
	param = 0;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_SET_BITWIDTH, (void *)&param);

	/* baudrate */
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_SET_CKR, (void *)&rate);

	/* mode */
	if (mode & BK_SPI_CPOL)
		param = 1;
	else
		param = 0;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_SET_CKPOL, (void *)&param);

	/* CPHA */
	if (mode & BK_SPI_CPHA)
		param = 1;
	else
		param = 0;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_SET_CKPHA, (void *)&param);

	/* Master */
	param = 1;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_SET_MSTEN, (void *)&param);

	/* 4 LINE MODE */
	param = 0;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_SET_NSSMD, (void *)&param);

	/*master int enable*/
	param = 1;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_INIT_MSTEN, (void *)&param);

	/* set call back func */
	spi_dev_cb.callback = bk_spi_rx_callback;
	spi_dev_cb.param = NULL;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_SET_RX_CALLBACK, (void *)&spi_dev_cb);

	spi_dev_cb.callback = bk_spi_tx_needwrite_callback;
	spi_dev_cb.param = NULL;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_SET_TX_NEED_WRITE_CALLBACK, (void *)&spi_dev_cb);

	spi_dev_cb.callback = bk_spi_tx_finish_callback;
	spi_dev_cb.param = NULL;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_SET_TX_FINISH_CALLBACK, (void *)&spi_dev_cb);

	/* enable tx int*/
	param = 1;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TXINT_EN, (void *)&param);

	/*enable tx finish int enable bit*/
	param = 1;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TXFINISH_INT_EN, (void *)&param);

	/* enable rx int*/
	param = 1;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_RXINT_EN, (void *)&param);

	/*enable rx finish int enable bit*/
	param = 1;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_RXFINISH_INT_EN, (void *)&param);

	/* enable spi unit*/
	param = 1;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_UNIT_ENABLE, (void *)&param);

	BK_SPI_PRT("spi_master:[CTRL]:0x%08x spi_channel:%d\r\n", REG_READ(SPI_CTRL(spi_channel)),spi_channel);
}

static void bk_spi_unconfigure(void)
{
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_DEINIT_MSTEN, NULL);
}

int bk_spi_master_xfer(struct spi_message *msg)
{
    UINT32 param, total_size;

    BK_ASSERT(spi_dev != NULL);
    BK_ASSERT(msg != NULL);

    rtos_lock_mutex(&spi_dev->mutex);

    total_size = msg->recv_len + msg->send_len;
    if(total_size)
    {
        GLOBAL_INT_DECLARATION();

        /* initial spi_dev */
        GLOBAL_INT_DISABLE();
        spi_dev->tx_ptr = msg->send_buf;
        spi_dev->tx_len = msg->send_len;

        spi_dev->rx_ptr = msg->recv_buf;
        spi_dev->rx_len = msg->recv_len;
        spi_dev->rx_offset = 0;
        spi_dev->rx_drop = msg->send_len;

        spi_dev->total_len = total_size;
        spi_dev->flag &= ~(TX_FINISH_FLAG);
        GLOBAL_INT_RESTORE();

        /*tx len*/
        param = msg->send_len;
        sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TXTRANS_LEN, (void *)&param);

        /*rx len*/
        param = msg->recv_len + msg->send_len;
        sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_RXTRANS_LEN, (void *)&param);

        sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TX_FIFO_CLR, NULL);
        sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_RX_FIFO_CLR, NULL);

        /* enable tx int*/
        param = 1;
        sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TXINT_EN, (void *)&param);

        /*enable tx finish int enable bit*/
        param = 1;
        sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TXFINISH_INT_EN, (void *)&param);

		BK_SPI_PRT("spi_master:[CTRL]:0x%08x \n", REG_READ(SPI_CTRL(spi_channel)));
        BK_SPI_PRT("spi_master [SPI_CN]:0x%08x\n", REG_READ(SPI_CONFIG(spi_channel)));

        BK_SPI_PRT("tx_len:%d,rx_len:%d,total_size:%d\r\n",msg->send_len,msg->recv_len,total_size);

        if(msg->recv_len)//need recv data
        {
            param = 1;
            sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TX_RX_EN, (void *)&param);
        }
        else
        {
            /* TX_EN enable */
            param = 1;
            sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TX_EN, (void *)&param);
        }

        /* wait tx finish */
        rtos_get_semaphore(&spi_dev->tx_sem, BEKEN_NEVER_TIMEOUT);

        /*disable tx rx*/
        param = 0;
        sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TX_RX_EN, (void *)&param);

        BK_SPI_PRT("master xfer finish\r\n");

        /* initial spi_dev with zero*/
        GLOBAL_INT_DISABLE();
        spi_dev->tx_ptr = NULL;
        spi_dev->tx_len = 0;

        spi_dev->rx_ptr = NULL;
        spi_dev->rx_len = 0;

        spi_dev->total_len = 0;
        spi_dev->flag |= TX_FINISH_FLAG;
        GLOBAL_INT_RESTORE();
    }

    rtos_unlock_mutex(&spi_dev->mutex);

    return msg->recv_len;
}

int bk_spi_master_init(UINT32 rate, UINT32 mode)
{
	bk_err_t result = 0;

	if (spi_dev)
		bk_spi_master_deinit();

	spi_dev = os_malloc(sizeof(struct bk_spi_dev));
	if (!spi_dev) {
		BK_SPI_PRT("[spi]:malloc memory for spi_dev failed\n");
		result = -1;
		goto _exit;
	}
	os_memset(spi_dev, 0, sizeof(struct bk_spi_dev));


	result = rtos_init_semaphore(&spi_dev->tx_sem, 1);
	if (result != kNoErr) {
		BK_SPI_PRT("[spi]: spi tx semp init failed\n");
		goto _exit;
	}

	result = rtos_init_mutex(&spi_dev->mutex);
	if (result != kNoErr) {
		BK_SPI_PRT("[spi]: spi mutex init failed\n");
		goto _exit;
	}

	spi_dev->tx_ptr = NULL;
	spi_dev->tx_len = 0;
	spi_dev->flag |= TX_FINISH_FLAG;

	bk_spi_configure(rate, mode);

	return 0;

_exit:
	if (spi_dev->mutex)
		rtos_deinit_mutex(&spi_dev->mutex);

	if (spi_dev->tx_sem)
		rtos_deinit_semaphore(&spi_dev->tx_sem);

	if (spi_dev) {
		os_free(spi_dev);
		spi_dev = NULL;
	}

	return 1;
}

int bk_spi_master_deinit(void)
{
	if (spi_dev == NULL)
		return 0;

	if (spi_dev->mutex)
		rtos_lock_mutex(&spi_dev->mutex);

	if (spi_dev->tx_sem)
		rtos_deinit_semaphore(&spi_dev->tx_sem);

	if (spi_dev->mutex) {
		rtos_unlock_mutex(&spi_dev->mutex);
		rtos_deinit_mutex(&spi_dev->mutex);
	}

	if (spi_dev) {
		os_free(spi_dev);
		spi_dev = NULL;
	}

	bk_spi_unconfigure();

	return 0;
}

#if CONFIG_SPI_DMA
extern volatile int dma_trans_flag ;

#define SPI_TEST_POART1		0
#define SPI_TEST_POART2		1
#define SPI_TX_BUFFER_SIZE		30*24
#define SPI_RX_BUFFER_SIZE		30*24
#define SPI_RX_DMA_CHANNEL     GDMA_CHANNEL_1
#define SPI_TX_DMA_CHANNEL     GDMA_CHANNEL_3

void spi_debug_prt(UINT32 dma_channel);
void spi_dma_tx_enable(UINT8 enable);
void spi_dma_rx_enable(UINT8 enable);
void spi_dma_tx_half_handler(UINT32 param);
void spi_dma_rx_half_handler(UINT32 param);
void bk_master_dma_rx_disable(void);

void bk_spi_dma_rx_finish_callback(UINT32 param)
{
	rtos_set_semaphore(&spi_dev->rx_sem);
	BK_SPI_PRT("spi_dma master rx finish handler\r\n");
}

void bk_spi_dma_tx_finish_callback(UINT32 param)
{
	if ((spi_dev->flag & TX_FINISH_FLAG) == 0)
	{
		spi_dev->flag |= TX_FINISH_FLAG;
		rtos_set_semaphore(&spi_dev->tx_sem);
	}

	BK_SPI_PRT("spi_dma master tx finish handler\r\n");
}

void spi_dma_master_tx_init(struct spi_message *spi_msg)
{
	GDMACFG_TPYES_ST init_cfg;
	GDMA_CFG_ST en_cfg;

	BK_SPI_PRT("spi dma tx init\r\n");
	os_memset(&init_cfg, 0, sizeof(GDMACFG_TPYES_ST));
	os_memset(&en_cfg, 0, sizeof(GDMA_CFG_ST));

	init_cfg.dstdat_width = 8;
	init_cfg.srcdat_width = 32;
	init_cfg.dstptr_incr =  0;
	init_cfg.srcptr_incr =  1;

	init_cfg.src_start_addr = spi_msg->send_buf;
	init_cfg.dst_start_addr = (void *)(SPI_DAT(spi_channel));

	init_cfg.channel = SPI_TX_DMA_CHANNEL ;
	init_cfg.prio = 0;
	init_cfg.u.type4.src_loop_start_addr = spi_msg->send_buf;
	init_cfg.u.type4.src_loop_end_addr = spi_msg->send_buf + spi_msg->send_len;

	init_cfg.half_fin_handler = spi_dma_tx_half_handler;
	init_cfg.fin_handler = bk_spi_dma_tx_finish_callback;

	if (spi_channel == 0)
		init_cfg.dst_module = GDMA_X_DST_GSPI1_TX_REQ;

	else if (spi_channel == 1)
		init_cfg.dst_module = GDMA_X_DST_GSPI2_TX_REQ;

	else if (spi_channel == 2)
		init_cfg.dst_module = GDMA_X_DST_GSPI3_TX_REQ;

	init_cfg.src_module = GDMA_X_SRC_DTCM_RD_REQ;
	sddev_control(DD_DEV_TYPE_GDMA, CMD_GDMA_CFG_TYPE4, (void *)&init_cfg);

	en_cfg.channel = SPI_TX_DMA_CHANNEL;
	en_cfg.param = spi_msg->send_len;	// dma translen
	sddev_control(DD_DEV_TYPE_GDMA, CMD_GDMA_SET_TRANS_LENGTH, (void *)&en_cfg);

	en_cfg.channel = SPI_TX_DMA_CHANNEL;
	en_cfg.param = 0;					// 0:not repeat 1:repeat
	sddev_control(DD_DEV_TYPE_GDMA, CMD_GDMA_CFG_WORK_MODE, (void *)&en_cfg);

	en_cfg.channel = SPI_TX_DMA_CHANNEL;
	en_cfg.param = 0; // src no loop
	sddev_control(DD_DEV_TYPE_GDMA, CMD_GDMA_CFG_SRCADDR_LOOP, &en_cfg);

}

void spi_dma_master_rx_init(struct spi_message *spi_msg)
{
	GDMACFG_TPYES_ST init_cfg;
	GDMA_CFG_ST en_cfg;

	BK_SPI_PRT("spi dma rx init\r\n");
	os_memset(&init_cfg, 0, sizeof(GDMACFG_TPYES_ST));
	os_memset(&en_cfg, 0, sizeof(GDMA_CFG_ST));

	init_cfg.dstdat_width = 32;
	init_cfg.srcdat_width = 8;
	init_cfg.dstptr_incr =  1;
	init_cfg.srcptr_incr =  0;

	init_cfg.src_start_addr = (void *)(SPI_DAT(spi_channel));
	init_cfg.dst_start_addr = spi_msg->recv_buf;

	init_cfg.channel = SPI_RX_DMA_CHANNEL;
	init_cfg.prio = 0;
	init_cfg.u.type5.dst_loop_start_addr = spi_msg->recv_buf;
	init_cfg.u.type5.dst_loop_end_addr = spi_msg->recv_buf + spi_msg->recv_len;

	init_cfg.half_fin_handler = spi_dma_rx_half_handler;
	init_cfg.fin_handler = bk_spi_dma_rx_finish_callback;

	if (spi_channel == 0)
		init_cfg.src_module = GDMA_X_SRC_GSPI1_RX_REQ;

	else if (spi_channel == 1)
		init_cfg.src_module = GDMA_X_SRC_GSPI2_RX_REQ;

	else if (spi_channel == 2)
		init_cfg.src_module = GDMA_X_SRC_GSPI3_RX_REQ;

	init_cfg.dst_module = GDMA_X_DST_DTCM_WR_REQ;

	sddev_control(DD_DEV_TYPE_GDMA, CMD_GDMA_CFG_TYPE5, (void *)&init_cfg);

	en_cfg.channel = SPI_RX_DMA_CHANNEL;
	en_cfg.param   = spi_msg->recv_len;		// dma translen
	sddev_control(DD_DEV_TYPE_GDMA, CMD_GDMA_SET_TRANS_LENGTH, (void *)&en_cfg);


	en_cfg.channel = SPI_RX_DMA_CHANNEL;
	en_cfg.param = 0;						// 0:not repeat 1:repeat
	sddev_control(DD_DEV_TYPE_GDMA, CMD_GDMA_CFG_WORK_MODE, (void *)&en_cfg);

}


void bk_spi_master_dma_config(UINT32 mode, UINT32 rate)
{
	UINT32 param;
	bk_spi_configure(rate, mode);

	//disable tx/rx int disable
	param = 0;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TXINT_EN, (void *)&param);

	param = 0;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_RXINT_EN, (void *)&param);

	//disable rx/tx finish enable bit
	param = 0;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TXFINISH_INT_EN, (void *)&param);

	param = 0;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_RXFINISH_INT_EN, (void *)&param);

	//disable rx/tx over
	param = 0;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_RXOVR_EN, (void *)&param);

	param = 0;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TXOVR_EN, (void *)&param);

	param = 0;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_SET_NSSMD, (void *)&param);

	param = 0;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_SET_BITWIDTH, (void *)&param);

	//disable CSN intterrupt
	param = 0;
	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_CS_EN, (void *)&param);

	BK_SPI_PRT("spi_master [CTRL]:0x%08x \n", REG_READ(SPI_CTRL(spi_channel)));
	BK_SPI_PRT("spi_master [CONFIG]:0x%08x \n", REG_READ(SPI_CONFIG(spi_channel)));

}


int bk_spi_master_dma_tx_init(UINT32 mode, UINT32 rate, struct spi_message *spi_msg)
{
	bk_err_t result = 0;
	UINT32 send_len= spi_msg->send_len;

	if (spi_dev)
		bk_spi_master_deinit();

	spi_dev = os_malloc(sizeof(struct bk_spi_dev));
	if (!spi_dev) {
		BK_SPI_PRT("[spi]:malloc memory for spi_dev failed\n");
		result = -1;
		goto _exit;
	}
	os_memset(spi_dev, 0, sizeof(struct bk_spi_dev));


	result = rtos_init_semaphore(&spi_dev->tx_sem, 1);
	if (result != kNoErr) {
		BK_SPI_PRT("[spi]: spi tx semp init failed\n");
		goto _exit;
	}

	bk_spi_master_dma_config(mode, rate);

	spi_dma_master_tx_init(spi_msg);

	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_TXTRANS_LEN, (void *)&send_len);

	return 0;

_exit:

	if (spi_dev->tx_sem)
		rtos_deinit_semaphore(&spi_dev->tx_sem);


	if (spi_dev) {
		os_free(spi_dev);
		spi_dev = NULL;
	}

	return 1;
}

int bk_spi_master_dma_rx_init(UINT32 mode, UINT32 rate, struct spi_message *spi_msg)
{
	bk_err_t result = 0;
	UINT32 recv_len= spi_msg->recv_len;

	if (spi_dev)
		bk_spi_master_deinit();

	spi_dev = os_malloc(sizeof(struct bk_spi_dev));
	if (!spi_dev) {
		BK_SPI_WPRT("[spi]:malloc memory for spi_dev failed\n");
		result = -1;
		goto _exit;
	}
	os_memset(spi_dev, 0, sizeof(struct bk_spi_dev));


	result = rtos_init_semaphore(&spi_dev->rx_sem, 1);
	if (result != kNoErr) {
		BK_SPI_WPRT("[spi]: spi tx semp init failed\n");
		goto _exit;
	}

	bk_spi_master_dma_config(mode, rate);

	spi_dma_master_rx_init(spi_msg);

	sddev_control(DD_DEV_TYPE_SPI_SLECT, CMD_SPI_RXTRANS_LEN, (void *)&recv_len);

	return 0;

_exit:

	if (spi_dev->rx_sem)
		rtos_deinit_semaphore(&spi_dev->rx_sem);


	if (spi_dev) {
		os_free(spi_dev);
		spi_dev = NULL;
	}

	return 1;
}

int bk_spi_master_dma_send(struct spi_message *spi_msg)
{
	UINT32 ret = 0;
	GLOBAL_INT_DECLARATION();
	BK_ASSERT(spi_msg != NULL);

	GLOBAL_INT_DISABLE();
	spi_dev->flag &= ~(TX_FINISH_FLAG);
	GLOBAL_INT_RESTORE();

	spi_dma_tx_enable(ENABLE);

	/* wait tx finish */
	rtos_get_semaphore(&spi_dev->tx_sem, BEKEN_NEVER_TIMEOUT);
	if (ret)
		BK_SPI_WPRT("spi_dma tx error:wait tx_sem\r\n");

	spi_dma_tx_enable(DISABLE);

	return ret;

}



int bk_spi_master_dma_recv(struct spi_message *spi_msg)
{
	UINT32 ret = 0;
	GLOBAL_INT_DECLARATION();
	BK_ASSERT(spi_msg != NULL);

	GLOBAL_INT_DISABLE();
	spi_dma_rx_enable(ENABLE);
	GLOBAL_INT_RESTORE();

	rtos_get_semaphore(&spi_dev->rx_sem, BEKEN_NEVER_TIMEOUT);
	if (ret)
		BK_SPI_WPRT("spi_dma tx error:wait tx_sem\r\n");

	spi_dma_rx_enable(DISABLE);

	return ret;

}

int bk_spi_master_dma_init(UINT32 mode, UINT32 rate, struct spi_message *spi_msg)
{
	UINT32 ret = 0;

	if (spi_msg->send_buf)
	{
		ret = bk_spi_master_dma_tx_init( mode, rate, spi_msg);
		if (ret)
			BK_SPI_WPRT("master:spi dma int error:%d\r\n", ret);
	}

	if (spi_msg->recv_buf)
	{
		ret = bk_spi_master_dma_rx_init( mode, rate, spi_msg);
		if (ret)
			BK_SPI_WPRT("master:spi dma init error:%d\r\n", ret);
	}

	return ret;
}

int bk_spi_master_dma_transfer(struct spi_message *spi_msg)
{
	UINT32 ret = 0;

	if (spi_msg->send_buf)
	{
		ret = bk_spi_master_dma_send(spi_msg);
		if (ret)
			BK_SPI_WPRT("spi dma send error:%d\r\n", ret);
	}

	if (spi_msg->recv_buf)
	{
		ret = bk_spi_master_dma_recv(spi_msg);
		if (ret)
			BK_SPI_WPRT("spi dma recv error:%d\r\n", ret);
	}

	return ret;
}

int bk_spi_dma_init(UINT32 mode, UINT32 rate, struct spi_message *spi_msg)
{
	UINT32 ret = 0;

	if (mode & SPI_SLAVE)
	{
		ret = bk_spi_slave_dma_init(mode, rate, spi_msg);
		if (ret)
			BK_SPI_WPRT("slave:spi dma int error:%d\r\n", ret);
	} else
	{
		ret = bk_spi_master_dma_init(mode, rate, spi_msg);
		if (ret)
			BK_SPI_WPRT("master:spi dma init error:%d\r\n", ret);
	}

	return ret;
}

int bk_spi_dma_transfer(UINT32 mode, struct spi_message *spi_msg)
{
	UINT32 ret = 0;

	if (mode & SPI_SLAVE)
	{
		ret = bk_spi_slave_dma_transfer(spi_msg);
		if (ret)
			BK_SPI_WPRT("slave:spi dma int error:%d\r\n", ret);
	} else
	{
		ret = bk_spi_master_dma_transfer(spi_msg);
		if (ret)
			BK_SPI_WPRT("master:spi dma init error:%d\r\n", ret);
	}


	return ret;
}


int bk_spi_dma_deinit(UINT32 mode)
{
	UINT32 ret = 0;

	if (mode & SPI_SLAVE)
	{
		ret = bk_spi_slave_deinit();
		if (ret)
			BK_SPI_WPRT("slave:spi dma deint error:%d\r\n", ret);
	} else
	{
		ret = bk_spi_master_deinit();
		if (ret)
			BK_SPI_WPRT("master:spi dma deinit error:%d\r\n", ret);
	}


	return ret;
}

#endif  //CONFIG_SPI_DMA
#endif  // CONFIG_SPI_MASTER
#endif  //(CONFIG_SOC_BK7271)

