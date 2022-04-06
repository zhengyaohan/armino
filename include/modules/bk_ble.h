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

//TODO
// 1. Add comments
// 2. Remove all extern
// 3. Code style
// 4. Naming
#include <common/sys_config.h>

#ifndef _BLE_API_5_X_H_
#define _BLE_API_5_X_H_
#include <common/bk_typedef.h>
#ifdef __cplusplus
extern"C" {
#endif

typedef bk_err_t ble_err_t;

#define ERR_SUCCESS               BK_OK
#define ERR_FAIL                  BK_FAIL
#define ERR_NO_MEM                BK_ERR_NO_MEM

#define ERR_PROFILE               (BK_ERR_BLE_BASE - 1)
#define ERR_CREATE_DB             (BK_ERR_BLE_BASE - 2)
#define ERR_CMD_NOT_SUPPORT       (BK_ERR_BLE_BASE - 3)
#define ERR_UNKNOW_IDX            (BK_ERR_BLE_BASE - 4)
#define ERR_BLE_STATUS            (BK_ERR_BLE_BASE - 5)
#define ERR_ADV_DATA              (BK_ERR_BLE_BASE - 6)
#define ERR_CMD_RUN               (BK_ERR_BLE_BASE - 7)
#define ERR_INIT_CREATE           (BK_ERR_BLE_BASE - 8)
#define ERR_INIT_STATE            (BK_ERR_BLE_BASE - 9)
#define ERR_ATTC_WRITE            (BK_ERR_BLE_BASE - 10)
#define ERR_ATTC_WRITE_UNREGISTER (BK_ERR_BLE_BASE - 11)

#if (CONFIG_BLE_HOST_RW)
#define MAX_ADV_DATA_LEN           (0x1F)
#define MAX_SCAN_NUM               (15)

/// BD address length
#define GAP_BD_ADDR_LEN       (6)
/// Maximal length of the Device Name value
#define APP_DEVICE_NAME_MAX_LEN      (18)

#define ABIT(n) (1 << n)

#define BK_PERM_SET(access, right) \
    (((BK_PERM_RIGHT_ ## right) << (access ## _POS)) & (access ## _MASK))

#define BK_PERM_GET(perm, access)\
    (((perm) & (access ## _MASK)) >> (access ## _POS))

typedef enum
{
	/// Stop notification/indication
	PRF_STOP_NTFIND = 0x0000,
	/// Start notification
	PRF_START_NTF,
	/// Start indication
	PRF_START_IND,
} prf_conf;


/**
 *   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * |EXT | WS | I  | N  | WR | WC | RD | B  |    NP   |    IP   |   WP    |    RP   |
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 *
 * Bit [0-1]  : Read Permission         (0 = NO_AUTH, 1 = UNAUTH, 2 = AUTH, 3 = SEC_CON)
 * Bit [2-3]  : Write Permission        (0 = NO_AUTH, 1 = UNAUTH, 2 = AUTH, 3 = SEC_CON)
 * Bit [4-5]  : Indication Permission   (0 = NO_AUTH, 1 = UNAUTH, 2 = AUTH, 3 = SEC_CON)
 * Bit [6-7]  : Notification Permission (0 = NO_AUTH, 1 = UNAUTH, 2 = AUTH, 3 = SEC_CON)
 *
 * Bit [8]    : Broadcast permission
 * Bit [9]    : Read Command accepted
 * Bit [10]   : Write Command accepted
 * Bit [11]   : Write Request accepted
 * Bit [12]   : Send Notification
 * Bit [13]   : Send Indication
 * Bit [14]   : Write Signed accepted
 * Bit [15]   : Extended properties present
 */

typedef enum
{
	/// Read Permission Mask
	RP_MASK             = 0x0003,
	RP_POS              = 0,
	/// Write Permission Mask
	WP_MASK             = 0x000C,
	WP_POS              = 2,
	/// Indication Access Mask
	IP_MASK            = 0x0030,
	IP_POS             = 4,
	/// Notification Access Mask
	NP_MASK            = 0x00C0,
	NP_POS             = 6,
	/// Broadcast descriptor present
	BROADCAST_MASK     = 0x0100,
	BROADCAST_POS      = 8,
	/// Read Access Mask
	RD_MASK            = 0x0200,
	RD_POS             = 9,
	/// Write Command Enabled attribute Mask
	WRITE_COMMAND_MASK = 0x0400,
	WRITE_COMMAND_POS  = 10,
	/// Write Request Enabled attribute Mask
	WRITE_REQ_MASK     = 0x0800,
	WRITE_REQ_POS      = 11,
	/// Notification Access Mask
	NTF_MASK           = 0x1000,
	NTF_POS            = 12,
	/// Indication Access Mask
	IND_MASK           = 0x2000,
	IND_POS            = 13,
	/// Write Signed Enabled attribute Mask
	WRITE_SIGNED_MASK  = 0x4000,
	WRITE_SIGNED_POS   = 14,
	/// Extended properties descriptor present
	EXT_MASK           = 0x8000,
	EXT_POS            = 15,
} bk_perm_mask;

/**
 * Attribute Extended permissions
 *
 * Extended Value permission bit field
 *
 *   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * | RI |UUID_LEN |EKS |                       Reserved                            |
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 *
 * Bit [0-11] : Reserved
 * Bit [12]   : Encryption key Size must be 16 bytes
 * Bit [13-14]: UUID Length             (0 = 16 bits, 1 = 32 bits, 2 = 128 bits, 3 = RFU)
 * Bit [15]   : Trigger Read Indication (0 = Value present in Database, 1 = Value not present in Database)
 */
typedef enum
{
	/// Check Encryption key size Mask
	EKS_MASK         = 0x1000,
	EKS_POS          = 12,
	/// UUID Length
	UUID_LEN_MASK    = 0x6000,
	UUID_LEN_POS     = 13,
	/// Read trigger Indication
	RI_MASK          = 0x8000,
	RI_POS           = 15,
}bk_ext_perm_mask;

/**
 * Service permissions
 *
 *    7    6    5    4    3    2    1    0
 * +----+----+----+----+----+----+----+----+
 * |SEC |UUID_LEN |DIS |  AUTH   |EKS | MI |
 * +----+----+----+----+----+----+----+----+
 *
 * Bit [0]  : Task that manage service is multi-instantiated (Connection index is conveyed)
 * Bit [1]  : Encryption key Size must be 16 bytes
 * Bit [2-3]: Service Permission      (0 = NO_AUTH, 1 = UNAUTH, 2 = AUTH, 3 = Secure Connect)
 * Bit [4]  : Disable the service
 * Bit [5-6]: UUID Length             (0 = 16 bits, 1 = 32 bits, 2 = 128 bits, 3 = RFU)
 * Bit [7]  : Secondary Service       (0 = Primary Service, 1 = Secondary Service)
 */
typedef enum
{
	/// Task that manage service is multi-instantiated
	SVC_MI_MASK        = 0x01,
	SVC_MI_POS         = 0,
	/// Check Encryption key size for service Access
	SVC_EKS_MASK       = 0x02,
	SVC_EKS_POS        = 1,
	/// Service Permission authentication
	SVC_AUTH_MASK      = 0x0C,
	SVC_AUTH_POS       = 2,
	/// Disable the service
	SVC_DIS_MASK       = 0x10,
	SVC_DIS_POS        = 4,
	/// Service UUID Length
	SVC_UUID_LEN_MASK  = 0x60,
	SVC_UUID_LEN_POS   = 5,
	/// Service type Secondary
	SVC_SECONDARY_MASK = 0x80,
	SVC_SECONDARY_POS  = 7,
}bk_svc_perm_mask;

/// Attribute & Service access mode
enum
{
	/// Disable access
	BK_PERM_RIGHT_DISABLE   = 0,
	/// Enable access
	BK_PERM_RIGHT_ENABLE   = 1,
};

/// Attribute & Service access rights
enum
{
	/// No Authentication
	BK_PERM_RIGHT_NO_AUTH  = 0,
	/// Access Requires Unauthenticated link
	BK_PERM_RIGHT_UNAUTH   = 1,
	/// Access Requires Authenticated link
	BK_PERM_RIGHT_AUTH     = 2,
	/// Access Requires Secure Connection link
	BK_PERM_RIGHT_SEC_CON  = 3,
};

/// Attribute & Service UUID Length
enum
{
	/// 16  bits UUID
	BK_PERM_RIGHT_UUID_16         = 0,
	/// 32  bits UUID
	BK_PERM_RIGHT_UUID_32         = 1,
	/// 128 bits UUID
	BK_PERM_RIGHT_UUID_128        = 2,
	/// Invalid
	BK_PERM_RIGHT_UUID_RFU        = 3,
};


typedef enum  {
	// ADV_CMD:FOR BLE 5.1
	BLE_CREATE_ADV,
	BLE_SET_ADV_DATA,
	BLE_SET_RSP_DATA,
	BLE_START_ADV,
	BLE_STOP_ADV,
	BLE_DELETE_ADV,
	// ADV_CMD:FOR BLE 4.2
	BLE_INIT_ADV,
	BLE_DEINIT_ADV,
	// SCAN_CMD:FOR BLE 5.1
	BLE_CREATE_SCAN,
	BLE_START_SCAN,
	BLE_STOP_SCAN,
	BLE_DELETE_SCAN,
	// SCAN_CMD:FOR BLE 4.2
	BLE_INIT_SCAN,
	BLE_DEINIT_SCAN,
	/////conn
	BLE_CONN_UPDATE_MTU,
	BLE_CONN_UPDATE_PARAM,
	BLE_CONN_DIS_CONN,
	BLE_CONN_READ_PHY,
	BLE_CONN_SET_PHY,

	////init
	BLE_INIT_CREATE,
	BLE_INIT_START_CONN,
	BLE_INIT_STOP_CONN,
	BLE_INIT_DIS_CONN,
	BLE_INIT_READ_CHAR,
	BLE_INIT_WRITE_CHAR,
	BLE_CMD_MAX,
} ble_cmd_t;

typedef enum  {
	BLE_5_STACK_OK,
	BLE_5_WRITE_EVENT,
	BLE_5_READ_EVENT,
	BLE_5_REPORT_ADV,
	BLE_5_MTU_CHANGE,
	BLE_5_CONNECT_EVENT,
	BLE_5_DISCONNECT_EVENT,
	BLE_5_ATT_INFO_REQ,
	BLE_5_CREATE_DB,

	BLE_5_TX_DONE,

	BLE_5_INIT_CONNECT_EVENT,
	BLE_5_INIT_DISCONNECT_EVENT,

	BLE_5_SDP_REGISTER_FAILED,
	BLE_5_READ_PHY_EVENT,
	BLE_5_CONN_UPDATA_EVENT,
} ble_notice_t;

typedef struct{
	uint8_t cmd_idx;      /**< The index of command */
	ble_err_t status;     /**< The status for this command */
}ble_cmd_param_t;

typedef struct
{
	uint8_t conn_idx;     /**< The index of the connection */
	uint16_t prf_id;      /**< The id of the profile */
	uint16_t att_idx;     /**< The index of the attribute */
	uint16_t length;      /**< The length of the attribute */
	uint8_t status;       /**< Use to know if it's possible to modify the attribute ,can contains authorization or application error code */
} att_info_req_t;

typedef struct
{
	uint8_t conn_idx;     /**< The index of the connection */
	uint16_t prf_id;      /**< The id of the profile */
	uint16_t att_idx;     /**< The index of the attribute */
	uint8_t *value;       /**< The attribute value */
	uint16_t len;         /**< The length of the attribute value */
} write_req_t;

typedef struct
{
	uint8_t conn_idx;     /**< The index of the connection */
	uint16_t prf_id;      /**< The id of the profile */
	uint16_t att_idx;     /**< The index of the attribute */
	uint8_t *value;       /**< The attribute value */
	uint16_t size;        /**< The size of attribute value to read*/
	uint16_t length;      /**< The data length read */
} read_req_t;

typedef struct
{
	uint8_t actv_idx;     /**< The index of the activity */
	uint8_t evt_type;     /**< Event type */
	uint8_t adv_addr_type;/**< Advertising address type: public/random */
	uint8_t adv_addr[6];  /**<Advertising address value */
	uint8_t data_len;     /**< Data length in advertising packet */
	uint8_t *data;        /**< Data of advertising packet */
	uint8_t rssi;         /**< RSSI value for advertising packet (in dBm, between -127 and +20 dBm) */
} recv_adv_t;

typedef struct
{
	uint8_t conn_idx;     /**< The index of connection */
	uint16_t mtu_size;    /**< The MTU size to exchange */
} mtu_change_t;

typedef struct
{
	/// The index of connection
	uint8_t conn_idx;
	/// Peer address type
	uint8_t peer_addr_type;
	/// Peer BT address
	uint8_t peer_addr[6];
} conn_ind_t;

typedef struct
{
	/// The index of connection
	uint8_t conn_idx;
	/// Reason of disconnection
    uint8_t reason;
} discon_ind_t;

typedef struct
{
	uint8_t status;      /**< The status for creating db */
	uint8_t prf_id;      /**< The id of the profile */
} create_db_t;

typedef struct
{
	/// 16 bits UUID LSB First
	uint8_t uuid[16];
	/// Attribute Permissions (@see enum attm_perm_mask)
	uint16_t perm;
	/// Attribute Extended Permissions (@see enum attm_value_perm_mask)
	uint16_t ext_perm;
	/// Attribute Max Size
	/// note: for characteristic declaration contains handle offset
	/// note: for included service, contains target service handle
	uint16_t max_size;
}bk_attm_desc_t;

struct bk_ble_db_cfg
{
	uint16_t prf_task_id;
	///Service uuid
	uint8_t uuid[16];
	///Number of db
	uint8_t att_db_nb;
	///Start handler, 0 means autoalloc
	uint16_t start_hdl;
	///Attribute database
	bk_attm_desc_t* att_db;
	///Service config
	uint8_t svc_perm;
};

struct adv_param {
	uint8_t  advData[MAX_ADV_DATA_LEN];   /**< Advertising data */
	uint8_t  advDataLen;                  /**< The length of advertising data */
	uint8_t  respData[MAX_ADV_DATA_LEN];  /**< Scan response data */
	uint8_t  respDataLen;                 /**< The length of scan response data */
	uint8_t  channel_map;                 /**< Advertising channel map */
	uint16_t interval_min;                /**< Minimum advertising interval */
	uint16_t interval_max;                /**< Maximum advertising interval */
	uint16_t duration;                    /**< Advertising duration */
};

struct scan_param {
	uint8_t  filter_en;                   /**< The control of filter */
	uint8_t  channel_map;                 /**< Channel mapping */
	uint16_t interval;                    /**< The scanning interval */
	uint16_t window;                      /**< The scanning window */
};

typedef struct {
	//TODO put customer specific init configuration here
} ble_init_config_t;

typedef struct{
	uint8_t addr[GAP_BD_ADDR_LEN];
}bd_addr_t;

typedef struct {
	uint8_t  tx_phy;                       /**< The transmitter PHY */
	uint8_t  rx_phy;                       /**< The receiver PHY */
} le_read_phy_t;

typedef struct {
	uint8_t  tx_phy;                       /**< The transmitter PHY */
	uint8_t  rx_phy;                       /**< The receiver PHY */
	uint8_t  phy_opt;                       /**< PHY options  */
} le_set_phy_t;

typedef struct
{
    /// Own address type:  public=0 / random=1 / rpa_or_pub=2 / rpa_or_rnd=3
    uint8_t own_addr_type;
        /// Advertising type (@see enum gapm_adv_type)
    uint8_t adv_type;
    /// Bit field indicating the channel mapping
    uint8_t chnl_map;
    /// Bit field value provided advertising properties (@see enum gapm_adv_prop for bit signification)
    uint16_t adv_prop;
    /// Minimum advertising interval (in unit of 625us). Must be greater than 20ms
    uint32_t adv_intv_min;
    /// Maximum advertising interval (in unit of 625us). Must be greater than 20ms
    uint32_t adv_intv_max;
    /// Indicate on which PHY primary advertising has to be performed (@see enum gapm_phy_type)
    /// Note that LE 2M PHY is not allowed and that legacy advertising only support LE 1M PHY
    uint8_t prim_phy;
    /// Indicate on which PHY secondary advertising has to be performed (@see enum gapm_phy_type)
    uint8_t second_phy;
}le_adv_param_t;

typedef struct
{
    /// Own address type (@see enum gapm_own_addr)
    uint8_t own_addr_type;
    /// on which the advertising packets should be received
    uint8_t scan_phy;
    /// Scan interval
    uint16_t scan_intv;
    /// Scan window
    uint16_t scan_wd;
}le_scan_param_t;

typedef struct
{
    /// Connection interval minimum
    uint16_t intv_min;
    /// Connection interval maximum
    uint16_t intv_max;
    /// Connection latency
    uint16_t con_latency;
    /// Link supervision timeout
    uint16_t sup_to;
    /// on which the advertising packets should be received on the primary advertising physical channel
    uint8_t init_phys;
}le_conn_param_t;

typedef void (*ble_cmd_cb_t)(ble_cmd_t cmd, ble_cmd_param_t *param);
typedef void (*ble_notice_cb_t)(ble_notice_t notice, void *param);


/**
 * @brief     Register a service 
 *
 * @param
 *    - ble_db_cfg: service param
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_create_db (struct bk_ble_db_cfg* ble_db_cfg);

/**
 * @brief     Register ble event notification callback
 *
 * @param
 *    - func: notification callback
 *
 * @return
 *    - void
 */
void ble_set_notice_cb(ble_notice_cb_t func);

/**
 * @brief     Get device name
 *
 * @param
 *    - name: store the device name
 *    - buf_len: the length of buf to store the device name
 *
 * @return
 *    - length: the length of device name
 */
uint8_t ble_appm_get_dev_name(uint8_t* name, uint32_t buf_len);

/**
 * @brief     Set device name
 *
 * @param
 *    - len: the length of device name
 *    - name: the device name to be set
 *
 * @return
 *    - length: the length of device name
 */
uint8_t ble_appm_set_dev_name(uint8_t len, uint8_t* name);

/**
 * @brief     Start a ble advertising
 *
 * @param
 *    - actv_idx: the index of activity
 *    - adv: the adv parameter
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_adv_start(uint8_t actv_idx, struct adv_param *adv, ble_cmd_cb_t callback);

/**
 * @brief     Stop the advertising that has been started
 *
 * @param
 *    - actv_idx: the index of activity
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_adv_stop(uint8_t actv_idx, ble_cmd_cb_t callback);

/**
 * @brief     Start a ble scan
 *
 * @param
 *    - actv_idx: the index of activity
 *    - scan: the scan parameter
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_scan_start(uint8_t actv_idx, struct scan_param *scan, ble_cmd_cb_t callback);

/**
 * @brief     Stop the scan that has been started
 *
 * @param
 *    - actv_idx: the index of activity
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_scan_stop(uint8_t actv_idx, ble_cmd_cb_t callback);

/**
 * @brief     Create a ble advertising activity
 *
 * @param
 *    - actv_idx: the index of activity
 *    - adv_param: the advertising parameter
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_create_advertising(uint8_t actv_idx, le_adv_param_t *adv_param, ble_cmd_cb_t callback);

/**
 * @brief     Start a ble advertising
 *
 * @param
 *    - actv_idx: the index of activity
 *    - duration: advertising duration
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_start_advertising(uint8_t actv_idx, uint16 duration, ble_cmd_cb_t callback);

/**
 * @brief     Stop the advertising that has been started
 *
 * @param
 *    - actv_idx: the index of activity
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_stop_advertising(uint8_t actv_idx, ble_cmd_cb_t callback);

/**
 * @brief     Delete the advertising that has been created
 *
 * @param
 *    - actv_idx: the index of activity
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_delete_advertising(uint8_t actv_idx, ble_cmd_cb_t callback);

/**
 * @brief     Set the advertising data
 *
 * @param
 *    - actv_idx: the index of activity
 *    - adv_buff: advertising data
 *    - adv_len: the length of advertising data
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_set_adv_data(uint8_t actv_idx, unsigned char* adv_buff, unsigned char adv_len, ble_cmd_cb_t callback);

/**
 * @brief     Set the scan response data
 *
 * @param
 *    - actv_idx: the index of activity
 *    - scan_buff: scan response data
 *    - scan_len: the length of scan response data
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_set_scan_rsp_data(uint8_t actv_idx, unsigned char* scan_buff, unsigned char scan_len, ble_cmd_cb_t callback);

/**
 * @brief     Set the periodic advertising data
 *
 * @param
 *    - actv_idx: the index of activity
 *    - per_adv_buff: periodic advertising data
 *    - per_adv_len: the length of periodic advertising data
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_set_per_adv_data(uint8_t actv_idx, unsigned char* per_adv_buff, unsigned char per_adv_len, ble_cmd_cb_t callback);

/**
 * @brief     Update connection parameters
 *
 * @param
 *    - conn_idx: the index of connection
 *    - conn_param: connection parameters
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_update_param(uint8_t conn_idx, le_conn_param_t *conn_param, ble_cmd_cb_t callback);

/**
 * @brief     Disconnect a ble connection
 *
 * @param
 *    - conn_idx: the index of connection
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_disconnect(uint8_t conn_idx, ble_cmd_cb_t callback);

/**
 * @brief     Exchange MTU
 *
 * @param
 *    - conn_idx: the index of connection
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_gatt_mtu_change(uint8_t conn_idx,ble_cmd_cb_t callback);

/**
 * @brief     Create a ble scan activity
 *
 * @param
 *    - actv_idx: the index of activity
 *    - scan_param: the scan parameter
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_create_scaning(uint8_t actv_idx, le_scan_param_t *scan_param, ble_cmd_cb_t callback);

/**
 * @brief     Start a ble scan
 *
 * @param
 *    - actv_idx: the index of activity
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_start_scaning(uint8_t actv_idx, ble_cmd_cb_t callback);

/**
 * @brief     Stop the scan that has been started
 *
 * @param
 *    - actv_idx: the index of activity
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_stop_scaning(uint8_t actv_idx, ble_cmd_cb_t callback);

/**
 * @brief     Delete the scan that has been created
 *
 * @param
 *    - actv_idx: the index of activity
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_delete_scaning(uint8_t actv_idx, ble_cmd_cb_t callback);
#if (CONFIG_BLE_SCAN_NUM || CONFIG_BLE_INIT_NUM)
/**
 * @brief     Create a activity for initiating a connection
 *
 * @param
 *    - con_idx: the index of connection
 *    - conn_param: the connection parameter
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_create_init(uint8_t con_idx, le_conn_param_t *conn_param, ble_cmd_cb_t callback);

/**
 * @brief     Initiate a connection
 *
 * @param
 *    - con_idx: the index of connection
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_init_start_conn(uint8_t con_idx, ble_cmd_cb_t callback);

/**
 * @brief     Stop a connection
 *
 * @param
 *    - con_idx: the index of connection
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_init_stop_conn(uint8_t con_idx,ble_cmd_cb_t callback);

/**
 * @brief     Set the address of the device to be connected
 *
 * @param
 *    - connidx: the index of connection
 *    - bdaddr: the address of the device to be connected
 *    - addr_type: the address type of the device to be connected
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_init_set_connect_dev_addr(uint8_t connidx, bd_addr_t *bdaddr, uint8_t addr_type);
#else
extern ble_err_t bk_ble_create_init(uint8_t con_idx, le_conn_param_t *conn_param, ble_cmd_cb_t callback);
extern ble_err_t bk_ble_init_start_conn(uint8_t con_idx, ble_cmd_cb_t callback);
extern ble_err_t bk_ble_init_stop_conn(uint8_t con_idx,ble_cmd_cb_t callback);
extern ble_err_t bk_ble_init_set_connect_dev_addr(uint8_t connidx, bd_addr_t *bdaddr, uint8_t addr_type);
#endif

/**
 * @brief     Get an idle activity
 *
 * @return
 *    - xx: the idle activity's index
 */
uint8_t bk_ble_get_idle_actv_idx_handle(void);

/**
 * @brief     Get the maximum number of activities
 *
 * @return
 *    - xx: the maximum number of activities
 */
uint8_t bk_ble_get_max_actv_handle(void);

/**
 * @brief     Get the maximum number of supported connections
 *
 * @return
 *    - xx: the maximum number of supported connections
 */
uint8_t bk_ble_get_max_conn_handle(void);

/**
 * @brief     Find the specific activity by activity's state
 *
 * @param
 *    - state: the activity's state
 *
 * @return
 *    - xx: the index of the activity in this state
 */
uint8_t bk_ble_find_actv_state_idx_handle(uint8_t state);

/**
 * @brief     Get an idle connection activity
 *
 * @return
 *    - xx: the idle connection activity's index
 */
uint8_t bk_ble_get_idle_conn_idx_handle(void);

/**
 * @brief     Find the specific initiating activity by activity's state
 *
 * @param
 *    - state: the initiating activity's state
 *
 * @return
 *    - xx: the index of the initiating activity in this state
 */
uint8_t bk_ble_find_master_state_idx_handle(uint8_t state);

/**
 * @brief     Find the specific connection activity by address
 *
 * @param
 *    - connt_addr: the address of the connected device
 *
 * @return
 *    - xx: the index of the connection activity meeting the address
 */
uint8_t bk_ble_find_conn_idx_from_addr(bd_addr_t *connt_addr);

/**
 * @brief     Get the connection state of the specific device
 *
 * @param
 *    - connt_addr: the device's address
 *
 * @return
 *    - 1: this device is connected
 *    - 0: this device is disconnected
 */
uint8_t bk_ble_get_connect_state(bd_addr_t * connt_addr);

#endif

#if (CONFIG_BLE_HOST_ZEPHYR)
#include "bluetooth/bluetooth.h"
#if (CONFIG_BLE_MESH_ZEPHYR)
#include "bluetooth/mesh.h"
#endif
#endif

/**
 * @brief     Init the BLE driver
 *
 * This API should be called before any other BLE APIs.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_init(void);

/**
 * @brief     Deinit the BLE driver
 *
 * This API free all resources related to BLE.
 *
 * @attention 1. This API is not ready yet, will support in future release.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_deinit(void);

/**
 * @brief     Enable ble power save
 *
 * @return
 *    - void
 */
extern void ble_ps_enable_set(void);

/**
 * @brief     Disable ble power save
 *
 * @return
 *    - void
 */
extern void ble_ps_enable_clear(void);

/**
 * @brief     Get ble power save state
 *
 * @return
 *    - 1: power save is enabled
 *    - 0: power save is disabled
 */
extern UINT32 ble_ps_enabled(void );

/**
 * @brief     get ble mac addr
 *
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_get_mac(uint8_t *mac);
/**
 * @brief     set ble mac addr
 *
  * @param
 *    - actv_idx: actv_idx: the index of activity
 *    - mac: the device's address
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_set_mac(uint8_t actv_idx, uint8_t *mac, ble_cmd_cb_t callback);

/**
 * @brief     send a notification of an attribute’s value
 *
  * @param
 *    - len: the length of attribute’s value
 *    - buf: attribute’s value
 *    - prf_id: The id of the profile
 *    - att_idx: The index of the attribute
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_send_ntf_value(uint32_t len, uint8_t *buf, uint16_t prf_id, uint16_t att_idx);

/**
 * @brief     get ble connection tx/rx phy info
 *
  * @param
 *    - conn_idx: the connection idx
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_read_phy(uint8_t conn_idx, ble_cmd_cb_t callback);

/**
 * @brief     set ble connection tx/rx phy info
 *
  * @param
 *    - conn_idx: the connection idx
 *    - phy_info: the set phy_info
 *    - callback: register a callback for this action
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_set_phy(uint8_t conn_idx, le_set_phy_t * phy_info, ble_cmd_cb_t callback);
#ifdef __cplusplus
}
#endif

#endif

