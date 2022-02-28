
#ifndef _shell_drv_h_
#define _shell_drv_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "bk_typedef.h"

typedef  enum
{
	bFALSE = 0,
	bTRUE  = !bFALSE,
} bool_t;

typedef enum
{
	SHELL_IO_CTRL_GET_STATUS = 0,
	SHELL_IO_CTRL_RX_RESET,
	SHELL_IO_CTRL_TX_RESET,
} shell_ctrl_cmd_t;


struct _shell_dev_drv;

typedef struct
{
	struct _shell_dev_drv  *dev_drv;
	u8      dev_type;
	void  * dev_ext;
} shell_dev_t;

typedef void  (* tx_complete_t)(u8 *pbuf, u16 Tag);
typedef void  (* rx_indicate_t)(void);

typedef struct _shell_dev_drv
{
	bool_t   (*init)(shell_dev_t * shell_dev);
	bool_t   (*open)(shell_dev_t * shell_dev, tx_complete_t tx_callback, rx_indicate_t rx_callback);
	u16      (*write_async)(shell_dev_t * shell_dev, u8 * pBuf, u16 BufLen, u16 Tag);
	u16      (*read)(shell_dev_t * shell_dev, u8 * pBuf, u16 BufLen);
	u16      (*write_sync)(shell_dev_t * shell_dev, u8 * pBuf, u16 BufLen);
	u16      (*write_echo)(shell_dev_t * shell_dev, u8 * pBuf, u16 BufLen);
	bool_t   (*io_ctrl)(shell_dev_t * shell_dev, u8 cmd, void * param);
	bool_t   (*close)(shell_dev_t * shell_dev);
} shell_dev_drv_t;

extern shell_dev_t		shell_uart;
extern shell_dev_t		shell_dev_mb;


#ifdef CONFIG_DUAL_CORE

#include "mailbox_channel.h"

struct _shell_ipc_drv;

typedef struct
{
	struct _shell_ipc_drv  *dev_drv;
	u8      dev_type;
	void  * dev_ext;
} shell_dev_ipc_t;

typedef int  (* shell_ipc_rx_t)(void *data_buf, u16 dataLen);

typedef struct _shell_ipc_drv
{
	bool_t   (*init)(shell_dev_ipc_t * dev_ipc);
	bool_t   (*open)(shell_dev_ipc_t * dev_ipc, shell_ipc_rx_t rx_callback);
	u16      (*read)(shell_dev_ipc_t * dev_ipc, u8 * pBuf, u16 BufLen);
	u16      (*write_sync)(shell_dev_ipc_t * dev_ipc, u8 * pBuf, u16 BufLen);
	bool_t   (*io_ctrl)(shell_dev_ipc_t * dev_ipc, u8 cmd, void * param);
	bool_t   (*close)(shell_dev_ipc_t * dev_ipc);
} shell_ipc_drv_t;

typedef struct
{
	mb_chnnl_hdr_t	hdr;
	u8			  * buf;
	u16				len;
	u16				tag;
} log_cmd_t;

typedef struct
{
	mb_chnnl_hdr_t	hdr;
	u8    * 		buf;
	u16     		len;
} user_cmd_t;

enum
{
	MB_CMD_LOG_OUT = 1,
	MB_CMD_LOG_OUT_OK,
	MB_CMD_USER_INPUT,
} ;

extern shell_dev_ipc_t		shell_dev_ipc;

#endif /* CONFIG_MASTER_CORE */

#ifdef __cplusplus
}
#endif

#endif /* _shell_drv_h_ */

