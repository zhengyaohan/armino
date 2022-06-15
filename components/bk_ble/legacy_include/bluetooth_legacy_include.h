#pragma once

//legacy
ble_err_t bk_ble_init(void);
uint8_t bk_ble_find_actv_state_idx_handle(uint8_t state);
uint8_t bk_ble_find_master_state_idx_handle(uint8_t state);

//ethermind
ble_err_t bk_ble_create_connection(ble_conn_param_normal_t *conn_param, ble_cmd_cb_t callback);
ble_err_t bk_ble_create_connection_ex(ble_conn_param_ex_t *conn_param, ble_cmd_cb_t callback);

ble_err_t bk_ble_send_notify
(
    ATT_HANDLE *att_handle,
    uint16_t service_handle,
    uint16_t char_handle,
    uint8_t *data,
    uint16_t len
);

ble_err_t bk_ble_att_write(ATT_HANDLE * att_handle, ATT_ATTR_HANDLE hdl, uint8_t * value, uint16_t length);
ble_err_t bk_ble_get_att_handle_from_device_handle(ATT_HANDLE *att_handle, DEVICE_HANDLE *device_handle);


ble_err_t bk_ble_gatt_db_add_service
           (
               /* IN */   GATT_DB_SERVICE_INFO * service_info,
               /* IN */   uint16_t                 num_attr_handles,
               /* OUT */  uint16_t               * service_handle
           );

ble_err_t bk_ble_gatt_db_add_characteristic
           (
               /* IN */  uint16_t              service_handle,
               /* IN */  GATT_DB_UUID_TYPE * char_uuid,
               /* IN */  uint16_t              perm,
               /* IN */  uint16_t              property,
               /* IN */  ATT_VALUE         * char_value,
               /* OUT */ uint16_t            * char_handle
           );


ble_err_t bk_ble_gatt_db_add_characteristic_descriptor
           (
               /* IN */  uint16_t              service_handle,
               /* IN */  uint16_t              char_handle,
               /* IN */  GATT_DB_UUID_TYPE * desc_uuid,
               /* IN */  uint16_t              perm,
               /* IN */  ATT_VALUE         * desc_value
           );

ble_err_t bk_ble_gatt_db_dyn_register(void);

ble_err_t bk_ble_gatt_db_init_pl(GATT_DB_PL_EXT_HANDLER_CB hndlr_cb);
void bk_ble_bt_gatt_db_get_char_val_hndl(GATT_DB_HANDLE *gdbh, ATT_ATTR_HANDLE *attr_handle);
