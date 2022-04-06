## ADK 6.1

### Features

* conditional compilation of HAP based on transport to reduce memory footprint
* features MFi token authentication
* enable leaving thread after unpair to comply with HAP specification

### Bug Fixes

* active characteristics of HeaterCooler and Faucet services shouldn't advertise timed write
* add build type and log level information to accessory info
* add sample code to turn on LED during identify routine for all sample applications
* change Stateless programmable switch to be a minimal thread device
* fix crash when invalidating HomeKit Data Stream
* fix unused function warning in Sensor release build
* increase max number of pending UARP TX messages to avoid running out of scratch buffers
* reduce the default number of simultaneous IP connections to 9 from previous value of 17
* reduce the sleep interval for sensor accessory to 2 seconds from previous value of 30 seconds
* refresh gateway address after three consecutive ping failures
* remove SED Thread shutdown delay when shutting down Thread due to network loss
* send connected event immediately upon subscription
* fix rare crash when invalidating HomeKit Data Stream

### Performance Improvements

* improve buffer logging performance
* improve HAPRawBuffer Zero() and CopyBytes() performance

### BREAKING CHANGES

* change the IIDs of the sensor services to avoid conflicts with other services

## ADK 6.0

### Features

* add a new log level to separate error/fault from default messages
* add app pairing state change callback
* add argument to set path and name for debug command file
* add backtrace to PAL abort
* add button/signal support for Lightbulb and Motion Sensor
* add button/signal support for lock target state toggle
* add CLI ability to toggle TV active/inactive
* add command line option to specify a HomeKitStore path name
* add compile flag to create battery powered camera recorder
* add configurable delay when simulating apply during firmware update
* add debug command to change battery low status for Lock app
* add diagnostics service support to all ADK sample applications
* add diagnostics support for nRF52 platform
* add Filter Maintenance service to Thermostat app
* add Raspberry Pi BLE peripheral manager
* add sample application for Security System
* add support for Access Code for Locks
* add support for NFC Access for Locks
* add support for Accessory Information in ADK logs and Diagnostics
* add support for Accessory Metrics
* add support for diagnostic text format
* add support to trigger pairing mode for accessories with NFC programmable tag using debug commands
* allow multiple BLE peripheral instances on macOS
* capture dmesg logs as part of diagnostics
* check if we are connected to Wifi to determine if WAC was successful
* enable notification context information
* improve mutex support in PAL
* log wifi info periodically
* macOS apps that use HAPPlatformCamera for recording no longer loop video
* macOS apps that use HAPPlatformCamera specify video file as a command at runtime
* ping IP gateway periodically
* plumb deny reasons for UARP platform asset check
* print backtrace upon segmentation fault
* support for HomeKit Data Stream version 1.1
* use static buffer for logging

### Bug Fixes

* accept empty characteristic write request
* accept path for specific clip to simulate camera on macOS
* add accessory configuration info to a separate diagnostics file
* add debug service to all the applications
* add macOS support to install.sh script
* add new compile flag HAP_FEATURE_FILE_STORAGE
* add some delay to clear config and enter WAC mode for the response to go over Wifi
* add support for UARP rescind all assets request
* add Thread id to hap logs
* address static analyzer issues in adaptive lighting
* allow camera to be configured either in Portrait or landscape mode
* always accommodate new IP connection
* always update BLE central connection during subscribe for macOS
* append multipart diagnostics logs before transferring diagnostics logs
* avoid long mp4 fragments by requesting IDR
* build with -Wextra warning
* call stop service discovery before disconnecting the session for bonjour goodbye to be sent
* change chunk size used for camera recorder mp4 fragments to 256K
* check target value against constraints for adaptive lighting
* choose correct datasend.open error response handler
* continue to receive all RTP packets for camera
* delayed DataSend.Close should not assume error
* deprecate the Lock Management service
* deregister mdns before deallocating
* deregister timer for configured message for WAC when we enter WAC mode or server stops
* disable broadcast event once un-acked till next connection
* do not treat reconfigure as fatal error
* do not wait for configured message to successfully complete WAC
* don't copy SEI into mp4 fragments
* enable -Wsign-compare
* enable and resolve pedantic compiler warnings
* enable diagnostics service for BLE and Thread
* enable minimal logs for release builds
* enable more static analyzer checks
* end camera streaming sessions before unpairing
* ensure all BLE characteristic and descriptor keys are unique
* ensure we get a valid HDS port for macOS
* exit the forked process on failure when running system command
* factor base36 into library
* factor HDS use into selecting session to kick out
* fix crash on invalidating dataSend stream via another protocol
* fix crash on late dataSend cancellation
* fix macOS PAL timer memory leak
* fix errors related to missing stdint definitions with some Xcode's
* fix event notifications for Adaptive Lighting color temp and brightness
* fix more Wunused-parameter warnings
* fix race condition in Diagnostics
* fix usage of HAPAssert in CompressZip
* flush logs on platform abort and segfaults
* flush logs to file before zipping
* handle dataSend.open protocolError as an error
* if power pulled while in connected state, GSN now increments on boot if char value changes
* ignore signals when we fork into IntermediateMain while bringing Soft AP up
* improve h264 bitstream parsing on macOS
* incorrect initialization of diagnostics module
* increase audio recorder queue size to support non-standard clips
* increase recorder queue sizes to allow higher bitrate
* increment GSN for purged broadcast events
* log message now includes changed Thread ID on same line
* fix macOS video processing race condition
* make HAPPlatformClockGetCurrent Thread safe
* merge Linux and Raspberry Pi docker images
* modify macOS audio sanity checks to address race condition
* populate packet time in TLV for Selected Audio Stream Configuration characteristic
* prevent deadlock in accessory diagnostics
* prevent unpaired state timer from leaking
* product data should not be a hidden characteristic
* provide a HAP API to open tcp listener and subsequently open hap sessions
* provide url parameters only for manufacturer snapshot
* reduce HDS over HAP chunk bytes to 1KB
* reduce the size of the generated QRCode
* remove all instance of mfi-config service
* remove assert that caused the deadlock problem on factory reset
* remove callback for AccessoryRuntimeInformationService Ping read
* remove CVO references
* remove deprecated ActivityInterval characteristics of AccessoryRuntimeInformation service
* remove deprecated HAPAudioCodecParameters.type
* remove deprecated HAPCharacteristicProperties.requiresAdminPermissions
* remove deprecated HAPPlatform.BLE.available and HAPPlatform.ip.available
* remove deprecated HAPPlatform.ip.WAC
* remove deprecated HAPPlatformAccessorySetupCapabilities APIs
* remove deprecated HAPPlatformGetCompatibilityVersion API
* remove deprecated HAPPlatformWiFiManagerGetCapabilities APIs
* remove deprecated HAPTargetControlDataStreamProtocol.storage
* remove deprecated kHAPServiceType_Lightbulb and kHAPServiceType_FanV2
* remove deprecated Raspberry Pi Emulator support
* remove HAP_FEATURE_ENABLED guards in HAPCharacteristicTypes.h
* remove legacy WAC code
* remove redundant check for snapshot type
* remove unnecessary sleep interval configuration from applications that don't support Thread
* remove unused TestIO functionality from ADK
* rename natural light to adaptive light
* replace DEBUG compile time feature flag with HAP_TESTING
* replace integer values with #define for logging levels
* replace PAL log spinlock with mutex
* reset number of retries to check for wifi status to 0 on WAC being successful
* resolve assert on delayed dataSend.Close
* resolve crash when invalidated port is used
* restrict UARP to admin controllers
* return error response when an invalid snapshot type is provided in HDS open request
* save and restore value of firmware update characteristic across reboots
* save cookie value when we get a simple update request with empty station config
* set correct value for numStreamAvailableCallbacks in VideoDoorbell app
* set default c++ compiler for macOS
* skip next tag when ignoring key element in OPACK reader
* spec change for diagnostics snapshot value Value changed to a bitmask
* stop service discovery before stopping software access point when accessory server stops
* store timer per stream and invalid when stream is suspended
* support multiple stream protocol rx buffers
* support numeric IP address in install script
* timestamp in ADK log line should accommodate microseconds
* update Accessory Runtime Information Ping characteristic to Paired Read permission
* update IID for Siri Service
* update log message when event notification is not sent
* use dispatch_source_t to handle signals on macOS
* use dumpmachine to get compiler target
* use HAPPlatformTCPStreamManagerGetWiFiCapability to set wifi capability in device IE bits
* use the Github URL for boringssl
* use unsigned short range for BLE object handle
* when Adaptive Light transitions are set, assume last notified value as the current value
