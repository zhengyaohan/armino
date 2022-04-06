BLE APIs
================

:link_to_translation:`en:[English]`

.. important::

   The BLE API v1.0 is the lastest stable BLE APIs. All new applications should use BLE API v1.0.


BLE API Categories
----------------------------

Most of BLE APIs can be categoried as:

Interface specific BLE APIs:
 - :cpp:func:`bk_ble_init` - init ble
 - :cpp:func:`bk_ble_deinit` - deinit ble
 - :cpp:func:`ble_set_notice_cb` - set ble event notification callback
 - :cpp:func:`ble_appm_get_dev_name` - get device name
 - :cpp:func:`ble_appm_set_dev_name` - set device name
 - :cpp:func:`bk_ble_adv_start` - start legacy ble adv
 - :cpp:func:`bk_ble_adv_stop` - stop legacy ble adv
 - :cpp:func:`bk_ble_scan_start` - start ble scan
 - :cpp:func:`bk_ble_scan_stop` - stop ble scan
 - :cpp:func:`bk_ble_create_advertising` - create extended adv
 - :cpp:func:`bk_ble_start_advertising` - start extended adv
 - :cpp:func:`bk_ble_stop_advertising` - stop extended adv
 - :cpp:func:`bk_ble_delete_advertising` - delete extended adv
 - :cpp:func:`bk_ble_set_adv_data` - set legacy ble adv data
 - :cpp:func:`bk_ble_set_scan_rsp_data` - set ble scan response data
 - :cpp:func:`bk_ble_set_per_adv_data` - set ble periodic adv data
 - :cpp:func:`bk_ble_update_param` - update le connection paramters
 - :cpp:func:`bk_ble_gatt_mtu_change` - change gatt mtu
 - :cpp:func:`bk_ble_create_scaning` - create extended scan
 - :cpp:func:`bk_ble_start_scaning` - start extended scan
 - :cpp:func:`bk_ble_stop_scaning` - stop extended scan
 - :cpp:func:`bk_ble_delete_scaning` - delete extended scan
 - :cpp:func:`bk_ble_create_init` - create connection paramters
 - :cpp:func:`bk_ble_init_start_conn` - start connection link
 - :cpp:func:`bk_ble_init_stop_conn` - stop connection link
 - :cpp:func:`bk_ble_init_set_connect_dev_addr` - set connection device address
 - :cpp:func:`bk_ble_get_idle_actv_idx_handle` - get idle activity index handle
 - :cpp:func:`bk_ble_get_max_actv_handle` - get max active index handle
 - :cpp:func:`bk_ble_get_max_actv_handle` - get max connection index handle
 - :cpp:func:`bk_ble_find_actv_state_idx_handle` - find active state index handle
 - :cpp:func:`bk_ble_get_idle_conn_idx_handle` - get idle connection index handle
 - :cpp:func:`bk_ble_find_master_state_idx_handle` - find master state index handle
 - :cpp:func:`bk_ble_find_conn_idx_from_addr` - find connection index from address
 - :cpp:func:`bk_ble_get_connect_state` - get connection state

Compitability and Extension
----------------------------------------

The BLE APIs are flexible, easy to be extended and backward compatible. For most of the BLE configurations, we put some reserved fields in the config struct for future extendence. The API users need to make sure the reserved fields are initialized to 0, otherwise the compatibility may be broken as more fields are added.

Programing Principle
----------------------------------------

.. important::
  Here is some general principle for BLE API users:
   - Always init the reserved fields of config stuct to 0
   - Use BK_ERR_CHECK to check the return value of the BLE API
   - If you are not sure how to use BLE APIs, study the BLE example code first
   - If you are not sure how to initialize some fields of config struct, use the default configuration macro to use the config first and then set application specific fields.
   - Don't do too much work in BLE event callback, relay the event to your own application task.

User Development Model
----------------------------------------

Similar as most popular BLE driver, the Beken BLE driver is implemented as event driver. The application call BLE APIs to operate the BLE driver and get notified by BLE event.

API Reference
----------------------------------------

.. include:: ../../_build/inc/bk_api_ble.inc
