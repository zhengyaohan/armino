# Fetch repos from 192.168.0.46
# TODO peter - should automatically fetch code locally or from github

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

set(MBEDCRYPTO_VERSION                  "91fbed6" CACHE STRING "The version of Mbed Crypto to use")
set(MBEDCRYPTO_GIT_REMOTE               "ssh://${USER}@192.168.0.46:29418/iot/wcn/components/tfm_mbedtls" CACHE STRING "The URL (or path) to retrieve MbedTLS from.")
set(MCUBOOT_VERSION                     "adc1670"  CACHE STRING    "The version of MCUboot to use")
# TODO peter - 
#set(MCUBOOT_IMAGE_VERSION_S             ${TFM_VERSION} CACHE STRING "Version number of S image")
set(MCUBOOT_IMAGE_VERSION_S             0.0.1 CACHE STRING "Version number of S image")
set(MCUBOOT_IMAGE_VERSION_NS            0.0.1       CACHE STRING    "Version number of NS image")

FetchContent_Declare(mcuboot
    GIT_REPOSITORY ssh://${USER}@192.168.0.46:29418/iot/wcn/bootloader/mcuboot
    GIT_TAG ${MCUBOOT_VERSION}
    GIT_SHALLOW FALSE
    GIT_PROGRESS TRUE
    GIT_SUBMODULES "${MCUBOOT_SUBMODULES}"
)

FetchContent_GetProperties(mcuboot)
if(NOT mcuboot_POPULATED)
    FetchContent_Populate(mcuboot)
    set(MCUBOOT_PATH ${mcuboot_SOURCE_DIR} CACHE PATH "Path to MCUBOOT (or DOWNLOAD to get automatically" FORCE)
endif()

set(TFM_TEST_REPO_VERSION               "66103cc"   CACHE STRING    "The version of tf-m-tests to use")
FetchContent_Declare(tfm_test_repo
    GIT_REPOSITORY ssh://${USER}@192.168.0.46:29418/iot/wcn/components/tfm_tests
    GIT_TAG ${TFM_TEST_REPO_VERSION}
    GIT_PROGRESS TRUE
)

set(TFM_TEST_REPO_VERSION               "66103cc"   CACHE STRING    "The version of tf-m-tests to use")
FetchContent_GetProperties(tfm_test_repo)
if(NOT tfm_test_repo_POPULATED)
    FetchContent_Populate(tfm_test_repo)
    set(TFM_TEST_REPO_PATH ${tfm_test_repo_SOURCE_DIR} CACHE PATH "Path to TFM-TEST repo (or DOWNLOAD to fetch automatically" FORCE)
endif()


