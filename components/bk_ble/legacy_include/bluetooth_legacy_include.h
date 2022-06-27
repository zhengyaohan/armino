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

ble_err_t bk_ble_set_advertising_params_extended(
                uint8_t   adv_handle,
                uint16_t  adv_event_properties,
                uint32_t  primary_advertising_interval_min,
                uint32_t  primary_advertising_interval_max,
                uint8_t   primary_advertising_channel_map,
                uint8_t   own_address_type,
                uint8_t   peer_address_type,
                uint8_t * peer_address,
                uint8_t   advertising_filter_policy,
                int8_t    advertising_tx_power,
                uint8_t   primary_advertising_phy,
                uint8_t   secondary_adv_max_skip,
                uint8_t   secondary_advertising_phy,
                uint8_t   advertising_set_id,
                uint8_t   scan_req_nfy_enable,
                ble_cmd_cb_t callback);
ble_err_t bk_ble_set_advertising_params(
                uint32_t  primary_advertising_interval_min,
                uint32_t  primary_advertising_interval_max,
                uint8_t   primary_advertising_channel_map,
                uint8_t   own_address_type,
                uint8_t   primary_advertising_phy,
                uint8_t   secondary_advertising_phy,
                ble_cmd_cb_t callback);

ble_err_t bk_ble_set_advertising_data_extended(
                uint8_t   advertising_handle,
                uint8_t   operation,
                uint8_t   frag_pref,
                uint8_t * adv_buff,
                uint8_t   adv_len,
                ble_cmd_cb_t callback);
ble_err_t bk_ble_set_advertising_data(
                uint8_t* adv_buff,
                uint8_t adv_len,
                ble_cmd_cb_t callback);

ble_err_t bk_ble_set_scan_response_data_extended(
                uint8_t   advertising_handle,
                uint8_t   operation,
                uint8_t   fragment_pref,
                uint8_t   scan_response_data_length,
                uint8_t * scan_response_data,
                ble_cmd_cb_t callback);
ble_err_t bk_ble_set_scan_response_data(
                uint8_t   scan_response_data_length,
                uint8_t * scan_response_data,
                ble_cmd_cb_t callback);

ble_err_t bk_ble_set_advertising_enable_extended(
                uint8_t    enable,
                uint8_t    number_of_sets,
                uint8_t  * advertising_handle,
                uint16_t * duration,
                uint8_t  * max_extd_adv_evts,
                ble_cmd_cb_t callback);
ble_err_t bk_ble_set_advertising_enable(
                uint8_t    enable,
                ble_cmd_cb_t callback);

ble_err_t bk_ble_set_scan_parameters_extended(
                uint8_t    own_address_type,
                uint8_t    scanning_filter_policy,
                uint8_t    scanning_phy,
                uint16_t * scan_interval,
                uint16_t * scan_window,
                ble_cmd_cb_t callback);
ble_err_t bk_ble_set_scan_parameters(
                uint8_t    own_address_type,
                uint8_t    scanning_filter_policy,
                uint8_t    scanning_phy,
                uint16_t   scan_interval,
                uint16_t   scan_window,
                ble_cmd_cb_t callback);

ble_err_t bk_ble_set_scan_enable_extended(
                uint8_t   enable,
                uint8_t   filter_duplicates,
                uint16_t  duration,
                uint16_t  period,
                ble_cmd_cb_t callback);


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
ble_err_t bk_ble_disconnect_connection(bd_addr_t *addr, uint8_t addr_type, ble_cmd_cb_t callback);
ble_err_t bk_ble_cancel_connect(ble_cmd_cb_t callback);
