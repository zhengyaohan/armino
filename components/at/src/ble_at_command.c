#include <stdio.h>
#include <stdlib.h>
#include "at_common.h"
#if (CONFIG_BLE_5_X || CONFIG_BTDM_5_2)
#include "at_ble_common.h"

static beken_semaphore_t ble_at_cmd_sema = NULL;
ble_err_t at_cmd_status =ERR_SUCCESS;

int set_ble_name_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int get_ble_name_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int ble_set_adv_param_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int ble_set_adv_data_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int ble_set_per_adv_data_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int ble_set_scan_rsp_data_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int ble_set_adv_enable_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int ble_set_scan_param_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int ble_set_scan_enable_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int ble_create_connect_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int ble_cancel_create_connect_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int ble_disconnect_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int ble_update_conn_param_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int ble_get_conn_state_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

const at_command_t ble_at_cmd_table[] = {
    {0, "SETBLENAME", 0, "return ble name", set_ble_name_handle},
    {1, "GETBLENAME", 0, "return ble name", get_ble_name_handle},
    {2, "SETADVPARAM", 1, "help", ble_set_adv_param_handle},
    {3, "SETADVDATA", 1, "set adv data", ble_set_adv_data_handle},
    {4, "SETPERADVDATA", 1, "set perodic adv data", ble_set_per_adv_data_handle},
    {5, "SETSCANRSPDATA", 1, "set scan response data", ble_set_scan_rsp_data_handle},
    {6, "SETADVENABLE", 1, "set adv enable(1)/disable(0)", ble_set_adv_enable_handle},
    {7, "SETSCANPARAM", 1, "set scan param", ble_set_scan_param_handle},
    {8, "SETSCANENABLE", 1, "set scan enable(1)/disable(0)", ble_set_scan_enable_handle},
#if (CONFIG_BLE_INIT_NUM)
    {9, "CREATECONNECT", 1, "create connection", ble_create_connect_handle},
    {10, "CANCELCONNECT", 1, "cancel create connection", ble_cancel_create_connect_handle},
    {11, "DISCONNECT", 1, "disconnect current connection", ble_disconnect_handle},
    {12, "UPDATECONNPARAM", 1, "update connection param", ble_update_conn_param_handle},
#endif
    {13, "GETCONNECTSTATE", 0, "get connection state", ble_get_conn_state_handle},
};

int ble_at_cmd_cnt(void)
{
    return sizeof(ble_at_cmd_table) / sizeof(ble_at_cmd_table[0]);
}

void ble_at_cmd_cb(ble_cmd_t cmd, ble_cmd_param_t *param)
{
    at_cmd_status = param->status;
    switch (cmd)
    {
        case BLE_CREATE_ADV:
        case BLE_SET_ADV_DATA:
        case BLE_SET_RSP_DATA:
        case BLE_START_ADV:
        case BLE_STOP_ADV:
        case BLE_CREATE_SCAN:
        case BLE_START_SCAN:
        case BLE_STOP_SCAN:
        case BLE_INIT_CREATE:
        case BLE_INIT_START_CONN:
        case BLE_INIT_STOP_CONN:
        case BLE_CONN_DIS_CONN:
        case BLE_CONN_UPDATE_PARAM:
            if (ble_at_cmd_sema != NULL)
                rtos_set_semaphore( &ble_at_cmd_sema );
            break;
        default:
            break;
    }

}

void ble_at_notice_cb(ble_notice_t notice, void *param)
{
    switch (notice) {
    case BLE_5_STACK_OK:
        os_printf("ble stack ok");
        break;
    case BLE_5_WRITE_EVENT: {
        write_req_t *w_req = (write_req_t *)param;
        os_printf("write_cb:conn_idx:%d, prf_id:%d, add_id:%d, len:%d, data[0]:%02x\r\n",
                w_req->conn_idx, w_req->prf_id, w_req->att_idx, w_req->len, w_req->value[0]);
        break;
    }
    case BLE_5_READ_EVENT: {
        read_req_t *r_req = (read_req_t *)param;
        os_printf("read_cb:conn_idx:%d, prf_id:%d, add_id:%d\r\n",
                r_req->conn_idx, r_req->prf_id, r_req->att_idx);
        r_req->value[0] = 0x12;
        r_req->value[1] = 0x34;
        r_req->value[2] = 0x56;
        r_req->length = 3;
        break;
    }
    case BLE_5_REPORT_ADV: {
        recv_adv_t *r_ind = (recv_adv_t *)param;
        uint8_t adv_type = r_ind->evt_type & 0x03;
        os_printf("r_ind:actv_idx:%d,", r_ind->actv_idx);
        switch (adv_type)
        {
            case 0:
                os_printf("evt_type:EXT_ADV,");
                break;
            case 1:
                os_printf("evt_type:LEG_ADV,");
                break;
            case 4:
                os_printf("evt_type:PER_ADV,");
                break;
            default:
                os_printf("evt_type:ERR_ADV,");
                break;
        }
        os_printf(" adv_addr_type:%d, adv_addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                r_ind->adv_addr_type, r_ind->adv_addr[0], r_ind->adv_addr[1], r_ind->adv_addr[2],
                r_ind->adv_addr[3], r_ind->adv_addr[4], r_ind->adv_addr[5]);
        break;
    }
    case BLE_5_MTU_CHANGE: {
        mtu_change_t *m_ind = (mtu_change_t *)param;
        os_printf("m_ind:conn_idx:%d, mtu_size:%d\r\n", m_ind->conn_idx, m_ind->mtu_size);
        break;
    }
    case BLE_5_CONNECT_EVENT: {
        conn_ind_t *c_ind = (conn_ind_t *)param;
        os_printf("c_ind:conn_idx:%d, addr_type:%d, peer_addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                c_ind->conn_idx, c_ind->peer_addr_type, c_ind->peer_addr[0], c_ind->peer_addr[1],
                c_ind->peer_addr[2], c_ind->peer_addr[3], c_ind->peer_addr[4], c_ind->peer_addr[5]);
        break;
    }
    case BLE_5_DISCONNECT_EVENT: {
        discon_ind_t *d_ind = (discon_ind_t *)param;
        os_printf("d_ind:conn_idx:%d,reason:%d\r\n", d_ind->conn_idx, d_ind->reason);
        break;
    }
    case BLE_5_ATT_INFO_REQ: {
        att_info_req_t *a_ind = (att_info_req_t *)param;
        os_printf("a_ind:conn_idx:%d\r\n", a_ind->conn_idx);
        a_ind->length = 128;
        a_ind->status = ERR_SUCCESS;
        break;
    }
    case BLE_5_CREATE_DB: {
        create_db_t *cd_ind = (create_db_t *)param;
        os_printf("cd_ind:prf_id:%d, status:%d\r\n", cd_ind->prf_id, cd_ind->status);
        break;
    }
    case BLE_5_INIT_CONNECT_EVENT: {
        conn_ind_t *c_ind = (conn_ind_t *)param;
        os_printf("BLE_5_INIT_CONNECT_EVENT:conn_idx:%d, addr_type:%d, peer_addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                c_ind->conn_idx, c_ind->peer_addr_type, c_ind->peer_addr[0], c_ind->peer_addr[1],
                c_ind->peer_addr[2], c_ind->peer_addr[3], c_ind->peer_addr[4], c_ind->peer_addr[5]);
        break;
    }
    case BLE_5_INIT_DISCONNECT_EVENT: {
        discon_ind_t *d_ind = (discon_ind_t *)param;
        os_printf("BLE_5_INIT_DISCONNECT_EVENT:conn_idx:%d,reason:%d\r\n", d_ind->conn_idx, d_ind->reason);
        break;
    }
    case BLE_5_SDP_REGISTER_FAILED:
        os_printf("BLE_5_SDP_REGISTER_FAILED\r\n");
        break;
    case BLE_5_READ_PHY_EVENT: {
        le_read_phy_t *phy_param = (le_read_phy_t *)param;
        os_printf("BLE_5_READ_PHY_EVENT:tx_phy:%02x, rx_phy:%02x\r\n",phy_param->tx_phy, phy_param->rx_phy);
        break;
    }
    default:
        break;
    }
}

int set_ble_name_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    //uint8_t name[] = "BK_BLE_7231n";
    uint8_t name_len = 0;/*os_strlen((const char *)name);*/
    int err = kNoErr;
    if (argc != 1)
    {
        err = kParamErr;
        goto error;
    }

    name_len = ble_appm_set_dev_name(os_strlen(argv[0]), (uint8_t *)argv[0]);
    if (name_len == 0)
    {
        os_printf("\nname is empty!!!\n");
        goto error;
    }

    msg = AT_CMD_RSP_SUCCEED;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return kNoErr;

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return err;
}


int get_ble_name_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    uint8_t name[APP_DEVICE_NAME_MAX_LEN] = {0};
    uint8_t name_len = 0;
    int err = kNoErr;
    if (argc != 0)
    {
        err = kParamErr;
        goto error;
    }

    name_len = ble_appm_get_dev_name(name, APP_DEVICE_NAME_MAX_LEN);
    if (name_len == 0)
    {
        os_printf("\nname is empty!!!\n");
        goto error;
    }

    sprintf(pcWriteBuffer, "%s%s", AT_CMDRSP_HEAD, name);
    return kNoErr;

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return err;
}

//AT+BLECMD=SETADVPARAM, map, min_intval, max_intval, local_addr_type, adv_type, adv_properties, prim_phy, second_phy
int ble_set_adv_param_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    int err = kNoErr;
    int actv_idx = 0;
    le_adv_param_t adv_param;

    if (argc != 8)
    {
        os_printf("\nThe count of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    os_memset(&adv_param, 0, sizeof(le_adv_param_t));
    adv_param.chnl_map = os_strtoul(argv[0], NULL, 16);
    if (adv_param.chnl_map > 7)
    {
        os_printf("\nThe first(channel_map) param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    adv_param.adv_intv_min = os_strtoul(argv[1], NULL, 16) & 0xFFFFFF;
    adv_param.adv_intv_max = os_strtoul(argv[2], NULL, 16) & 0xFFFFFF;
    if ((adv_param.adv_intv_min > ADV_INTERVAL_MAX || adv_param.adv_intv_min < ADV_INTERVAL_MIN)
        || (adv_param.adv_intv_max > ADV_INTERVAL_MAX || adv_param.adv_intv_max < ADV_INTERVAL_MIN)
        || (adv_param.adv_intv_min > adv_param.adv_intv_max))
    {
        os_printf("input param interval is error\n");
        err = kParamErr;
        goto error;
    }

    adv_param.own_addr_type = os_strtoul(argv[3], NULL, 16) & 0xFF;
    switch (adv_param.own_addr_type)
    {
        case 0:
        case 1:
            adv_param.own_addr_type = BLE_STATIC_ADDR;
            break;
        case 2:
            adv_param.own_addr_type = BLE_GEN_RSLV_ADDR;
            break;
        case 3:
            adv_param.own_addr_type = BLE_GEN_NON_RSLV_ADDR;
            break;
        default:
            os_printf("\nThe third(own_addr_type) param is wrong!\n");
            err = kParamErr;
            break;
    }

    if (err != kNoErr)
        goto error;

    adv_param.adv_type = os_strtoul(argv[4], NULL, 16) & 0xFF;
    if (adv_param.adv_type > 2)
    {
        os_printf("\nThe forth(adv_type) param is wrong!\n");
        err = kParamErr;
        goto error;
    }
    adv_param.adv_prop = os_strtoul(argv[5], NULL, 16) & 0xFFFF;
    adv_param.prim_phy = os_strtoul(argv[6], NULL, 16) & 0xFF;
    if(!(adv_param.prim_phy == 1 || adv_param.prim_phy == 3))
    {
        os_printf("input param prim_phy is error\n");
        err = kParamErr;
        goto error;
    }

    adv_param.second_phy = os_strtoul(argv[7], NULL, 16) & 0xFF;
    if(adv_param.second_phy < 1 || adv_param.second_phy > 3)
    {
        os_printf("input param second_phy is error\n");
        err = kParamErr;
        goto error;
    }

    if (ble_at_cmd_table[2].is_sync_cmd)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_CREATED);
    if (actv_idx == AT_BLE_MAX_ACTV)
    {
        actv_idx = bk_ble_get_idle_actv_idx_handle();
        if (actv_idx == UNKNOW_ACT_IDX)
        {
            err = kNoResourcesErr;
            goto error;
        }
    }

    err = bk_ble_create_advertising(actv_idx, &adv_param, ble_at_cmd_cb);
    if (err != ERR_SUCCESS)
        goto error;
    if(ble_at_cmd_sema != NULL) {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if(err != kNoErr) {
            goto error;
        } else {
            if (at_cmd_status == ERR_SUCCESS)
            {
                msg = AT_CMD_RSP_SUCCEED;
                os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if (ble_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    return err;
}

//AT+BLECMD=SETADVDATA,data,length
int ble_set_adv_data_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    uint8_t adv_data[31];
    uint8_t adv_len = 0;
    int actv_idx = -1;
    int err = kNoErr;

    if (argc != 2)
    {
        os_printf("input param error\n");
        err = kParamErr;
        goto error;
    }

    adv_len = os_strtoul(argv[1], NULL, 16) & 0xFF;
    if (adv_len > 31 || adv_len != os_strlen(argv[0]) / 2)
    {
        os_printf("input adv len over limited\n");
        err = kParamErr;
        goto error;
    }

    at_set_data_handle(adv_data, argv[0],  os_strlen(argv[0]));

    if (ble_at_cmd_table[3].is_sync_cmd)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_CREATED);

    if (actv_idx == AT_BLE_MAX_ACTV)
    {
        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_STARTED);
        if (actv_idx == AT_BLE_MAX_ACTV)
        {
            os_printf("ble adv not set params before\n");
            err = kNoResourcesErr;
            goto error;
        }
    }

    err = bk_ble_set_adv_data(actv_idx, adv_data, adv_len, ble_at_cmd_cb);
    if (err != ERR_SUCCESS)
        goto error;
    if(ble_at_cmd_sema != NULL) {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if(err != kNoErr) {
            goto error;
        } else {
            if (at_cmd_status == ERR_SUCCESS)
            {
                msg = AT_CMD_RSP_SUCCEED;
                os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if (ble_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    return err;
}

//AT+BLECMD=SETPERADVDATA,data,length
int ble_set_per_adv_data_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    uint8_t adv_data[31];
    uint8_t adv_len = 0;
    int actv_idx = -1;
    int err = kNoErr;

    if (argc != 2)
    {
        os_printf("input param error\n");
        err = kParamErr;
        goto error;
    }

    adv_len = os_strtoul(argv[1], NULL, 16) & 0xFF;
    if (adv_len > 31 || adv_len != os_strlen(argv[0]) / 2)
    {
        os_printf("input adv len over limited\n");
        err = kParamErr;
        goto error;
    }

    at_set_data_handle(adv_data, argv[0],  os_strlen(argv[0]));

    if (ble_at_cmd_table[4].is_sync_cmd)
    {
    err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_CREATED);
    if (actv_idx == AT_BLE_MAX_ACTV)
    {
        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_STARTED);
        if (actv_idx == AT_BLE_MAX_ACTV)
        {
            os_printf("ble adv not set params before\n");
            err = kNoResourcesErr;
            goto error;
        }
    }

    err = bk_ble_set_per_adv_data(actv_idx, adv_data, adv_len, ble_at_cmd_cb);
    if (err != ERR_SUCCESS)
        goto error;
    if(ble_at_cmd_sema != NULL) {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if(err != kNoErr) {
            goto error;
        } else {
            if (at_cmd_status == ERR_SUCCESS)
            {
                msg = AT_CMD_RSP_SUCCEED;
                os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if (ble_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    return err;
}

int ble_set_scan_rsp_data_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    uint8_t scan_rsp_data[31];
    uint8_t data_len = 0;
    int actv_idx = -1;
    int err = kNoErr;

    if (argc != 2)
    {
        os_printf("input param error\n");
        err = kParamErr;
        goto error;
    }

    data_len = os_strtoul(argv[1], NULL, 16) & 0xFF;
    if (data_len > 31 || data_len != os_strlen(argv[0]) / 2)
    {
        os_printf("input adv len over limited\n");
        err = kParamErr;
        goto error;
    }

    at_set_data_handle(scan_rsp_data, argv[0], os_strlen(argv[0]));

    if (ble_at_cmd_table[5].is_sync_cmd)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_CREATED);
    if (actv_idx == AT_BLE_MAX_ACTV)
    {
        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_STARTED);
        if (actv_idx == AT_BLE_MAX_ACTV)
        {
            os_printf("ble adv not set params before\n");
            err = kNoResourcesErr;
            goto error;
        }
    }

    err = bk_ble_set_scan_rsp_data(actv_idx, scan_rsp_data, data_len, ble_at_cmd_cb);
    if (err != ERR_SUCCESS)
        goto error;
    if(ble_at_cmd_sema != NULL) {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if(err != kNoErr) {
            goto error;
        } else {
            if (at_cmd_status == ERR_SUCCESS)
            {
                msg = AT_CMD_RSP_SUCCEED;
                os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if (ble_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    return err;

}

int ble_set_adv_enable_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    int actv_idx = -1;
    int enable = 0;
    int err = kNoErr;

    if (argc != 1)
    {
        os_printf("input param error\n");
        err = kParamErr;
        goto error;
    }

    if (os_strcmp(argv[0], "1") == 0)
    {
        enable = 1;
    }
    else if (os_strcmp(argv[0], "0") == 0)
    {
        enable = 0;
    }
    else
    {
        os_printf("the input param is error\n");
        err = kParamErr;
        goto error;
    }

    if (ble_at_cmd_table[6].is_sync_cmd)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    if (enable == 1)
    {
        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_CREATED);
    }
    else
    {
        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_STARTED);
    }

    if (actv_idx == AT_BLE_MAX_ACTV)
    {
        os_printf("ble adv not set params before\n");
        err = kNoResourcesErr;
        goto error;
    }

    if (enable == 1)
    {
        err = bk_ble_start_advertising(actv_idx, 0, ble_at_cmd_cb);
    }
    else
    {
        err = bk_ble_stop_advertising(actv_idx, ble_at_cmd_cb);
    }

    if (err != ERR_SUCCESS)
        goto error;
    if(ble_at_cmd_sema != NULL) {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if(err != kNoErr) {
            goto error;
        } else {
            if (at_cmd_status == ERR_SUCCESS)
            {
                msg = AT_CMD_RSP_SUCCEED;
                os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if (ble_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    return err;
}

int ble_set_scan_param_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    int err = kNoErr;
    le_scan_param_t scan_param;
    uint8_t actv_idx = 0;

    if (argc != 4)
    {
        os_printf("\nThe number of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    os_memset(&scan_param, 0, sizeof(le_scan_param_t));
    scan_param.own_addr_type = os_strtoul(argv[0], NULL, 16) & 0xFF;
    switch (scan_param.own_addr_type)
    {
        case 0:
        case 1:
            scan_param.own_addr_type = BLE_STATIC_ADDR;
            break;
        case 2:
            scan_param.own_addr_type = BLE_GEN_RSLV_ADDR;
            break;
        case 3:
            scan_param.own_addr_type = BLE_GEN_NON_RSLV_ADDR;
            break;
        default:
            os_printf("\nThe fourth param is wrong!\n");
            err = kParamErr;
            break;
    }

    if (err != kNoErr)
        goto error;

    scan_param.scan_phy = os_strtoul(argv[1], NULL, 16) & 0xFF;
    if (!(scan_param.scan_phy & (PHY_1MBPS_BIT | PHY_CODED_BIT)))
    {
        os_printf("\nThe scan phy param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    scan_param.scan_intv = os_strtoul(argv[2], NULL, 16) & 0xFFFF;
    scan_param.scan_wd = os_strtoul(argv[3], NULL, 16) & 0xFFFF;
    if (scan_param.scan_intv < SCAN_INTERVAL_MIN || scan_param.scan_intv > SCAN_INTERVAL_MAX ||
        scan_param.scan_wd < SCAN_WINDOW_MIN || scan_param.scan_wd > SCAN_WINDOW_MAX ||
        scan_param.scan_intv < scan_param.scan_wd)
    {
        os_printf("\nThe second/third param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    if (ble_at_cmd_table[7].is_sync_cmd)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    ble_set_notice_cb(ble_at_notice_cb);
    actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_SCAN_CREATED);
    if (actv_idx == AT_BLE_MAX_ACTV)
    {
        actv_idx = bk_ble_get_idle_actv_idx_handle();
        if (actv_idx == UNKNOW_ACT_IDX)
        {
            err = kNoResourcesErr;
            goto error;
        }
    }

    err = bk_ble_create_scaning(actv_idx, &scan_param, ble_at_cmd_cb);

    if (err != ERR_SUCCESS)
        goto error;
    if(ble_at_cmd_sema != NULL) {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if(err != kNoErr) {
            goto error;
        } else {
            if (at_cmd_status == ERR_SUCCESS)
            {
                msg = AT_CMD_RSP_SUCCEED;
                os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return err;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if (ble_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    return err;
}

int ble_set_scan_enable_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    uint8_t actv_idx = 0;
    int enable = 0;
    int err = kNoErr;

    if (argc != 1)
    {
        os_printf("input param error\n");
        err = kParamErr;
        goto error;
    }

    if (os_strcmp(argv[0], "1") == 0)
    {
        enable = 1;
    }
    else if (os_strcmp(argv[0], "0") == 0)
    {
        enable = 0;
    }
    else
    {
        os_printf("the input param is error\n");
        err = kParamErr;
        goto error;
    }

    if (ble_at_cmd_table[8].is_sync_cmd)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }
    ble_set_notice_cb(ble_at_notice_cb);

    if (enable == 1)
    {
        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_SCAN_CREATED);
    }
    else
    {
        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_SCAN_STARTED);
    }

    if (actv_idx == AT_BLE_MAX_ACTV)
    {
        os_printf("scan actv not start before\n");
        err = kNoResourcesErr;
        goto error;
    }

    if (enable == 1)
    {
        err = bk_ble_start_scaning(actv_idx, ble_at_cmd_cb);
    }
    else
    {
        err = bk_ble_stop_scaning(actv_idx, ble_at_cmd_cb);
    }

    if (err != ERR_SUCCESS)
        goto error;
    if(ble_at_cmd_sema != NULL) {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if(err != kNoErr) {
            goto error;
        } else {
            if (at_cmd_status == ERR_SUCCESS)
            {
                msg = AT_CMD_RSP_SUCCEED;
                os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if (ble_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    return err;
}

int ble_start_connect_handle(uint8_t actv_idx, uint8_t peer_addr_type, bd_addr_t *bdaddr, ble_cmd_cb_t cb)
{
    int err = kNoErr;
    err = bk_ble_init_set_connect_dev_addr(actv_idx, bdaddr, peer_addr_type);
    if (err != 0)
    {
        return err;
    }
    err = bk_ble_init_start_conn(actv_idx, cb);
    if (err != 0)
    {
        return err;
    }

    if(ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if(err == kNoErr)
        {
            if (at_cmd_status == ERR_SUCCESS)
            {
                return kNoErr;
            }
            else
            {
                err = at_cmd_status;
            }
        }
    }
    else
        err = kNotFoundErr;

    return err;
}

int ble_create_connect_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    uint8_t actv_idx = 0;
    le_conn_param_t conn_param;
    uint8_t peer_addr_type = 0;
    bd_addr_t bdaddr;
    int err = kNoErr;

    if (argc != 6)
    {
        os_printf("input param error\n");
        err = kParamErr;
        goto error;
    }

    conn_param.intv_min = os_strtoul(argv[0], NULL, 16) & 0xFFFF;
    conn_param.con_latency = os_strtoul(argv[1], NULL, 16) & 0xFFFF;
    if (conn_param.intv_min < CON_INTERVAL_MIN || conn_param.intv_min > CON_INTERVAL_MAX
        || conn_param.con_latency > CON_LATENCY_MAX)
    {
        err = kParamErr;
        goto error;
    }

    conn_param.sup_to = os_strtoul(argv[2], NULL, 16) & 0xFFFF;
    if (conn_param.sup_to < CON_SUP_TO_MIN || conn_param.sup_to > CON_SUP_TO_MAX)
    {
        err = kParamErr;
        goto error;
    }

    conn_param.init_phys = os_strtoul(argv[3], NULL, 16) & 0xFF;
    if (conn_param.init_phys > (PHY_1MBPS_BIT | PHY_2MBPS_BIT | PHY_CODED_BIT))
    {
        err = kParamErr;
        goto error;
    }

    peer_addr_type = os_strtoul(argv[4], NULL, 16) & 0xFF;
    if (peer_addr_type > 0x03)
    {
        err = kParamErr;
        goto error;
    }

    err = get_addr_from_param(&bdaddr, argv[5]);
    if (err != kNoErr)
    {
        os_printf("input param error\n");
        err = kParamErr;
        goto error;
    }

    if ((10 * conn_param.sup_to) < (((1 + conn_param.con_latency) * conn_param.intv_min * 5 + 1) >> 1))
    {
        os_printf("input param not suitable, maybe you can set con_latency to 0\n");
        err = kParamErr;
        goto error;
    }

    if (ble_at_cmd_table[9].is_sync_cmd)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    ble_set_notice_cb(ble_at_notice_cb);

    actv_idx = bk_ble_find_master_state_idx_handle(AT_INIT_STATE_CREATED);
    if (actv_idx == AT_BLE_MAX_CONN)
    {
        /// Do not create actv
        actv_idx = bk_ble_get_idle_conn_idx_handle();
        if (actv_idx == UNKNOW_ACT_IDX)
        {
            err = kNoResourcesErr;
            goto error;
        }

        err = bk_ble_create_init(actv_idx, &conn_param, ble_at_cmd_cb);
        if (err != ERR_SUCCESS)
            goto error;
        if(ble_at_cmd_sema != NULL) {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if(err != kNoErr) {
                goto error;
            } else {
                if (at_cmd_status == ERR_SUCCESS)
                {
                    err = ble_start_connect_handle(actv_idx, peer_addr_type, &bdaddr, ble_at_cmd_cb);
                    if (err != kNoErr)
                        goto error;
                    msg = AT_CMD_RSP_SUCCEED;
                    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return err;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }
    }
    else
    {
        /// have created actv, this happend in which connection have been disconnected
        err = ble_start_connect_handle(actv_idx, peer_addr_type, &bdaddr, ble_at_cmd_cb);
        if (err != kNoErr)
            goto error;
        msg = AT_CMD_RSP_SUCCEED;
        os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
        rtos_deinit_semaphore(&ble_at_cmd_sema);
        return err;
    }

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if (ble_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    return err;
}

int ble_cancel_create_connect_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    uint8_t actv_idx = 0;
    int err = kNoErr;

    if (argc != 0)
    {
        os_printf("input param error\n");
        err = kParamErr;
        goto error;
    }

    if (ble_at_cmd_table[10].is_sync_cmd)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    actv_idx = bk_ble_find_master_state_idx_handle(AT_INIT_STATE_CONECTTING);
    if (actv_idx == AT_BLE_MAX_ACTV)
    {
        os_printf("ble adv not set params before\n");
        err = kNoResourcesErr;
        goto error;
    }

    err = bk_ble_init_stop_conn(actv_idx, ble_at_cmd_cb);
    if (err != ERR_SUCCESS)
        goto error;
    if(ble_at_cmd_sema != NULL) {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if(err != kNoErr) {
            goto error;
        } else {
            if (at_cmd_status == ERR_SUCCESS)
            {
                msg = AT_CMD_RSP_SUCCEED;
                os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if (ble_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    return err;
}

int ble_disconnect_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    int err = kNoErr;
    bd_addr_t connect_addr;
    uint8_t conn_idx;

    if (argc != 1)
    {
        os_printf("input param error\n");
        err = kParamErr;
        goto error;
    }

    err = get_addr_from_param(&connect_addr, argv[0]);
    if (err != kNoErr)
    {
        os_printf("input addr param error\n");
        err = kParamErr;
        goto error;
    }

    if (ble_at_cmd_table[11].is_sync_cmd)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    /// get connect_idx from connect_addr
    conn_idx = bk_ble_find_conn_idx_from_addr(&connect_addr);
    if (conn_idx == AT_BLE_MAX_CONN)
    {
        os_printf("ble not connection\n");
        err = kNoResourcesErr;
        goto error;
    }

    err = bk_ble_disconnect(conn_idx, ble_at_cmd_cb);
    if (err != ERR_SUCCESS)
        goto error;
    if(ble_at_cmd_sema != NULL) {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if(err != kNoErr) {
            goto error;
        } else {
            if (at_cmd_status == ERR_SUCCESS)
            {
                msg = AT_CMD_RSP_SUCCEED;
                os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if (ble_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    return err;
}

int ble_update_conn_param_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    int err = kNoErr;
    bd_addr_t connect_addr;
    le_conn_param_t conn_param;
    uint8_t conn_idx;

    if (argc != 5)
    {
        os_printf("input param error\n");
        err = kParamErr;
        goto error;
    }

    err = get_addr_from_param(&connect_addr, argv[0]);
    if (err != kNoErr)
    {
        os_printf("input addr param error\n");
        err = kParamErr;
        goto error;
    }

    conn_param.intv_min = os_strtoul(argv[1], NULL, 16) & 0xFFFF;
    conn_param.intv_max = os_strtoul(argv[2], NULL, 16) & 0xFFFF;
    conn_param.con_latency = os_strtoul(argv[3], NULL, 16) & 0xFFFF;
    conn_param.sup_to = os_strtoul(argv[4], NULL, 16) & 0xFFFF;

    if ((conn_param.intv_min < CON_INTERVAL_MIN || conn_param.intv_min > CON_INTERVAL_MAX) ||
        (conn_param.intv_max < CON_INTERVAL_MIN || conn_param.intv_max > CON_INTERVAL_MAX) ||
        (conn_param.intv_min > conn_param.intv_max) || (conn_param.con_latency > CON_LATENCY_MAX) ||
        (conn_param.sup_to < CON_SUP_TO_MIN || conn_param.sup_to > CON_SUP_TO_MAX))
    {
        os_printf("input update param not suitable\n");
        err = kParamErr;
        goto error;
    }

    if ((10 * conn_param.sup_to) < (((1 + conn_param.con_latency) * conn_param.intv_max * 5 + 1) >> 1))
    {
        os_printf("input param not suitable, maybe you can set con_latency to 0\n");
        err = kParamErr;
        goto error;
    }

    if (ble_at_cmd_table[12].is_sync_cmd)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    /// get connect_idx from connect_addr
    conn_idx = bk_ble_find_conn_idx_from_addr(&connect_addr);
    if (conn_idx == AT_BLE_MAX_CONN)
    {
        os_printf("ble not connection\n");
        err = kNoResourcesErr;
        goto error;
    }

    err = bk_ble_update_param(conn_idx, &conn_param, ble_at_cmd_cb);
    if (err != ERR_SUCCESS)
        goto error;
    if(ble_at_cmd_sema != NULL) {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if(err != kNoErr) {
            goto error;
        } else {
            if (at_cmd_status == ERR_SUCCESS)
            {
                msg = AT_CMD_RSP_SUCCEED;
                os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return err;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if (ble_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    return err;
}

int ble_get_conn_state_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    int err = kNoErr;
    uint8_t conn_state = 0;
    bd_addr_t peer_addr;

    if (argc != 1)
    {
        os_printf("input param error\n");
        err = kParamErr;
        goto error;
    }

    err = get_addr_from_param(&peer_addr, argv[0]);
    if (err != kNoErr)
    {
        os_printf("input param error\n");
        err = kParamErr;
        goto error;
    }

    conn_state = bk_ble_get_connect_state(&peer_addr);

    if (conn_state == 1)
    {
        sprintf(pcWriteBuffer, "%s%s\r\n", AT_CMDRSP_HEAD, "BLE_CONNECT");
    }
    else
    {
        sprintf(pcWriteBuffer, "%s:%s\r\n", AT_CMDRSP_HEAD, "BLE_DISCONNECT");
    }

    return err;

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return err;
}

#endif
