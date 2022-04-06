## Compile Time Options

Command                          | Description
-------------------------------- | -------------------------------------------------------------------
make ? | <ul><li>apps - Build all apps (Default)</li></li><li>test - Build unit tests</li><li>all - Build apps and unit tests</li></ul>
make DEST=? install | Install applications and tools in a user specified directory. Default is the `Output` directory.
make APPS=? | Space delimited names of the app to compile. <br><br>Example: `make APPS=“Lightbulb Lock”`<br><br> Default: Build all applications
make PROFILE=? | Name of the profile to choose while building an application. <br><br>Example: `make PROFILE=Lightbulb`<br><br> Difference between `APPS` and `PROFILE` is in what features get enabled during build time. The `PROFILE` option attempts to build a minimal HAP, PAL, and application for a given HAP profile. While `APPS` option leaves most features in HAP and PAL enabled while building. For production purposes `PROFILE` option should be used to build an application.
make BUILD_TYPE=? | Build type: <br><ul><li>Debug (Default)</li><li>Test</li><li>Release</li></ul>
make CRYPTO=? | Supported cryptographic libraries: <br><ul><li>OpenSSL (Default)</li><li>MbedTLS</li><li>BoringSSL (Linux & Raspberry Pi)</li></ul>Example: `make CRYPTO=MbedTLS apps`
make DOCKER=? | Build with or without Docker: <br><ul><li>1 - Enable Docker during compilation (Default)</li><li>0 - Disable Docker during compilation</li></ul>
make ENABLE_BUFFER_LOGS=? | Enable buffer logs:<br><ul><li>0 - Disable</li><li>1 - Enable (Default)</li></ul>
make ENABLE_HAP_TESTING=? | Enable certain test features:<br><ul><li>0 - Disable (Default)</li><li>1 - Enable</li></ul>
make LOG_LEVEL=level | <ul><li>0 - No logs are displayed</li><li>1	- Error and Fault logs are displayed (Setting for test and release builds)</li><li>2 - Error, Fault and Default logs are displayed</li><li>3 - Error, Fault, Default and Info logs are displayed</li><li>4 - Error, Fault, Default, Info and Debug logs are displayed (Setting for debug build)</li></ul>
make PROTOCOLS=? | Space delimited protocols supported by the applications: <br><ul><li>BLE</li><li>IP</li></ul>Example: `make PROTOCOLS=“IP BLE”`<br><br>Default: All protocols
make TARGET=? | Build for a given target platform:<br><ul><li>macOS</li><li>Linux</li><li>nRF52</li></li><li>Raspberry Pi</li></ul>
make TESTS=? | Space delimited names of the tests to run. <br><br>Example: `make TESTS=“HAPTLVTest HAPUUIDTest”`<br><br>Default: Run all tests
make USE_DISPLAY=? | Build with display support enabled:<br><ul><li>0 - Disable (Default)</li><li>1 - Enable</li></ul>
make USE_PORTRAIT_MODE=? | Build camera with portrait mode sensor:<br><ul><li>0 - Disable (Default)</li><li>1 - Enable</li></ul>
make USE_HW_AUTH=? | Build with hardware authentication enabled: <br><ul><li>0 - Disable (Default)</li><li>1 - Enable</li></ul>
make USE_TOKEN_AUTH=? | Build with MFi software token authentication enabled: <br><ul><li>0 - Disable</li><li>1 - Enable (Default)</li></ul>
make USE_NFC=? | Build with NFC enabled:<br><ul><li>0 - Disable (Default)</li><li>1 - Enable</li></ul>
make USE_WAC=? | Build with WAC enabled:<br><ul><li>0 - Disable (Default)</li><li>1 - Enable</li></ul>
make USE_BATTERY_POWERED_CAMERA_RECORDER=? | Build Camera Recorder as Battery Powered. This is ignored for non-camera applications. Supported values:<br><ul><li>0 - Disable (Default)</li><li>1 - Enable</li></ul>
make CODESIGN=? | When building for macOS, instructs the build system to sign the resulting apps/libraries. <br><ul><li>0 - Disable (Default)</li><li>1 - Enable</li></ul>
make SENSOR=? | Name of the sensor to build: <br><ul><li>AirQuality</li><li>CarbonDioxide</li><li>CarbonMonoxide</li><li>Contact</li><li>Humidity</li><li>Leak</li><li>Light</li><li>Motion</li><li>Occupancy</li><li>Smoke</li><li>Temperature</li></ul>
make HAP_ADAPTIVE_LIGHT=? | Build with Adaptive Lighting enabled:<br><ul><li>0 - Disable (Default)</li><li>1 - Enable</li></ul>
make HAP_VIDEODOORBELL_SILENT_MODE=? | Build with or without doorbell silent mode:<br><ul><li>0 - Disable (Default)</li><li>1 - Enable</li></ul>
make HAP_VIDEODOORBELL_OPERATING_STATE=? | Build with or without doorbell operating state response:<br><ul><li>0 - Disable (Default)</li><li>1 - Enable</li></ul>
make HAP_WIFI_RECONFIGURATION=? | Build with Wifi Reconfiguration feature:<br><ul><li>0 - Disable (Default)</li><li>1 - Enable</li></ul>
make HAP_THREAD_DECOMMISSION_ON_UNPAIR=? | Build an application which will 1. Suppress unpaired thread advertisements and 2. (Lightbulb only) Shut down the accessory server and decommission thread after unpairing with a controller:<br><ul><li>0 - Disable (Default)</li><li>1 - Enable</li></ul>
