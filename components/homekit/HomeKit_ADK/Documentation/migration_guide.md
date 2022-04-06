ADK Migration Guide
===================

## From ADK 5.x to ADK 6.x
- ADK Integration Guide is replaced by this HTML documentation.
- Modified the default button/Unix signal behavior for sample applications used for debugging purposes.
- Enabled additional compiler warning flags such as *-Wextra*, *-wundef*, *-Wpedantic* and *-Wsign-compare*.
- Enabled *ccache* for faster compilation.
- Added BLE support for Rasbperry Pi.
- Removed legacy WAC support.
- Renamed compiler flag *HAP_THREAD_CERTIFICATION_OVERRIDES* to *HAP_FEATURE_THREAD_CERTIFICATION_OVERRIDES*.
- Miscellaneous stability and usability fixes for IPCameras.
- Added support for Accessory Diagnostics.
- Added support for Accessory Metrics.
- Added support for Accessory Firmware Update.
- Added support for Access Code for Locks.
- Added support for NFC Access for Locks.
- Breaking Changes:
  - Removed the following deprecated APIs:
    - *HAPAudioCodecParameters.type*
    - *HAPCharacteristicProperties.requiresAdminPermissions*
    - *HAPPlatform.ble.available* and *HAPPlatform.ip.available*
    - *HAPPlatform.ip.wac*
    - *HAPPlatformAccessorySetupCapabilities* APIs
    - *HAPPlatformGetCompatibilityVersion* API
    - *HAPPlatformWiFiManagerGetCapabilities* APIs
    - *HAPTargetControlDataStreamProtocol.storage*
    - *kHAPServiceType_Lightbulb* and *kHAPServiceType_FanV2*
  - Updated IID for Siri Service for Remote Application.
  - Renamed *HAP_SIRI* feature macro to *HAP_SIRI_REMOTE* to be more descriptive.
  - Removed all deprecated HAP_ENUMs.

## Migrating from Non-ADK HomeKit Implementations to ADK
When migrating from a different HomeKit SDK to the Apple ADK, certain steps need to be followed:

- The various aid and iid values in the HomeKit attribute database must match the previously used values. In the ADK
sample code those values can be adjusted in the DB.c file. It is important to keep those values the same across firmware
updates to prevent security issues and to avoid losing user-configured automations.
- The firmwareVersion value in the HAPAccessory structure needs to be higher than the previously used value.
- The accessory identity and pairing keys need to be imported into ADK before the initial *HAPAccessoryServerCreate* call.
The functions *HAPLegacyImportDeviceID*, *HAPLegacyImportConfigurationNumber*, *HAPLegacyImportLongTermSecretKey*,
*HAPLegacyImportUnsuccessfulAuthenticationAttemptsCounter*, and *HAPLegacyImportControllerPairing* may be used for this
purpose (see HAP.h). This ensures that the accessory remains paired after updating the firmware.
