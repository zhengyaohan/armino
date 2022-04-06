// Disclaimer: IMPORTANT: This Apple software is supplied to you, by Apple Inc. ("Apple"), in your
// capacity as a current, and in good standing, Licensee in the MFi Licensing Program. Use of this
// Apple software is governed by and subject to the terms and conditions of your MFi License,
// including, but not limited to, the restrictions specified in the provision entitled "Public
// Software", and is further subject to your agreement to the following additional terms, and your
// agreement that the use, installation, modification or redistribution of this Apple software
// constitutes acceptance of these additional terms. If you do not agree with these additional terms,
// you may not use, install, modify or redistribute this Apple software.
//
// Subject to all of these terms and in consideration of your agreement to abide by them, Apple grants
// you, for as long as you are a current and in good-standing MFi Licensee, a personal, non-exclusive
// license, under Apple's copyrights in this Apple software (the "Apple Software"), to use,
// reproduce, and modify the Apple Software in source form, and to use, reproduce, modify, and
// redistribute the Apple Software, with or without modifications, in binary form, in each of the
// foregoing cases to the extent necessary to develop and/or manufacture "Proposed Products" and
// "Licensed Products" in accordance with the terms of your MFi License. While you may not
// redistribute the Apple Software in source form, should you redistribute the Apple Software in binary
// form, you must retain this notice and the following text and disclaimers in all such redistributions
// of the Apple Software. Neither the name, trademarks, service marks, or logos of Apple Inc. may be
// used to endorse or promote products derived from the Apple Software without specific prior written
// permission from Apple. Except as expressly stated in this notice, no other rights or licenses,
// express or implied, are granted by Apple herein, including but not limited to any patent rights that
// may be infringed by your derivative works or by other works in which the Apple Software may be
// incorporated. Apple may terminate this license to the Apple Software by removing it from the list
// of Licensed Technology in the MFi License, or otherwise in accordance with the terms of such MFi License.
//
// Unless you explicitly state otherwise, if you provide any ideas, suggestions, recommendations, bug
// fixes or enhancements to Apple in connection with this software ("Feedback"), you hereby grant to
// Apple a non-exclusive, fully paid-up, perpetual, irrevocable, worldwide license to make, use,
// reproduce, incorporate, modify, display, perform, sell, make or have made derivative works of,
// distribute (directly or indirectly) and sublicense, such Feedback in connection with Apple products
// and services. Providing this Feedback is voluntary, but if you do provide Feedback to Apple, you
// acknowledge and agree that Apple may exercise the license granted above without the payment of
// royalties or further consideration to Participant.

// The Apple Software is provided by Apple on an "AS IS" basis. APPLE MAKES NO WARRANTIES, EXPRESS OR
// IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR
// IN COMBINATION WITH YOUR PRODUCTS.
//
// IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
// AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
// (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (C) 2015-2021 Apple Inc. All Rights Reserved.
#include <unistd.h>

#include "HAPPlatformMFiHWAuth+Init.h"
#include "HAPPlatformMFiHWAuth.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "MFiHWAuth" };

#if !(HAP_FEATURE_ENABLED(HAP_FEATURE_MFI_HW_AUTH))
void HAPPlatformMFiHWAuthCreate(HAPPlatformMFiHWAuthRef mfiHWAuth HAP_UNUSED) {
    HAPLogError(&logObject, "[HW Authentication disabled] %s.", __func__);
    HAPFatalError();
}

void HAPPlatformMFiHWAuthRelease(HAPPlatformMFiHWAuthRef mfiHWAuth HAP_UNUSED) {
    HAPLogError(&logObject, "[HW Authentication disabled] %s.", __func__);
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
bool HAPPlatformMFiHWAuthIsPoweredOn(HAPPlatformMFiHWAuthRef mfiHWAuth HAP_UNUSED) {
    HAPLogError(&logObject, "[HW Authentication disabled] %s.", __func__);
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformMFiHWAuthPowerOn(HAPPlatformMFiHWAuthRef mfiHWAuth HAP_UNUSED) {
    HAPLogError(&logObject, "[HW Authentication disabled] %s.", __func__);
    HAPFatalError();
}

void HAPPlatformMFiHWAuthPowerOff(HAPPlatformMFiHWAuthRef mfiHWAuth HAP_UNUSED) {
    HAPLogError(&logObject, "[HW Authentication disabled] %s.", __func__);
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformMFiHWAuthWrite(
        HAPPlatformMFiHWAuthRef mfiHWAuth HAP_UNUSED,
        const void* bytes HAP_UNUSED,
        size_t numBytes HAP_UNUSED) {
    HAPLogError(&logObject, "[HW Authentication disabled] %s.", __func__);
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformMFiHWAuthRead(
        HAPPlatformMFiHWAuthRef mfiHWAuth HAP_UNUSED,
        uint8_t registerAddress HAP_UNUSED,
        void* bytes HAP_UNUSED,
        size_t numBytes HAP_UNUSED) {
    HAPLogError(&logObject, "[HW Authentication disabled] %s.", __func__);
    HAPFatalError();
}
#else

#define kMCP2221_VendorID  ((unsigned short) 1240)
#define kMCP2221_ProductID ((unsigned short) 221)

#define kMCP2221_HIDReportSize   ((size_t) 64)
#define kMCP2221_HIDReportNumber ((uint8_t) 0)
#define kMCP2221_I2CAddress      ((uint8_t) 0x10)

/**
 * MCP2221 command code.
 */
HAP_ENUM_BEGIN(uint8_t, MCP2221CommandCode) {
    /**
     * Status/Set Parameters.
     */
    kMCP2221CommandCode_StatusSetParameters = 0x10,

    /**
     * Get I2C Data.
     */
    kMCP2221CommandCode_GetI2CData = 0x40,

    /**
     * I2C Write Data.
     */
    kMCP2221CommandCode_I2CWriteData = 0x90,

    /**
     * I2C Read Data.
     */
    kMCP2221CommandCode_I2CReadData = 0x91,
} HAP_ENUM_END(uint8_t, MCP2221CommandCode);

/**
 * MCP2221 status code.
 *
 * - These status codes are not documented in the specification.
 */
HAP_ENUM_BEGIN(uint8_t, MCP2221I2CStatus) {
    /**
     * Idle.
     */
    kMCP2221I2CStatus_Idle = 0x00,

    /**
     * Nack.
     */
    kMCP2221I2CStatus_Nack = 0x25,

    /**
     * Write in progress.
     */
    kMCP2221I2CStatus_Writing1 = 0x41,

    /**
     * Write in progress.
     */
    kMCP2221I2CStatus_Writing2 = 0x42,

    /**
     * Read in progress.
     */
    kMCP2221I2CStatus_Reading1 = 0x50,

    /**
     * Read in progress.
     */
    kMCP2221I2CStatus_Reading2 = 0x53,

    /**
     * Data is ready (partial data available).
     */
    kMCP2221I2CStatus_DataReadyPartial = 0x54,

    /**
     * Data is ready (full data available).
     */
    kMCP2221I2CStatus_DataReady = 0x55,

    /**
     * Operation in progress.
     */
    kMCP2221I2CStatus_InProgress = 0x61,
} HAP_ENUM_END(uint8_t, MCP2221I2CStatus);

void HAPPlatformMFiHWAuthCreate(HAPPlatformMFiHWAuthRef mfiHWAuth) {
    HAPPrecondition(mfiHWAuth);

    HAPLogDebug(&logObject, "Storage configuration: mfiHWAuth = %lu", (unsigned long) sizeof *mfiHWAuth);

    int err = hid_init();
    if (err) {
        HAPLogError(&logObject, "hid_init failed: %d", err);
        HAPFatalError();
    }

    mfiHWAuth->mcp2221 = NULL;
}

void HAPPlatformMFiHWAuthRelease(HAPPlatformMFiHWAuthRef mfiHWAuth) {
    HAPPrecondition(mfiHWAuth);

    if (mfiHWAuth->mcp2221) {
        hid_close(mfiHWAuth->mcp2221);
        mfiHWAuth->mcp2221 = NULL;
    }

    // Not doing hid_exit to prevent killing other active connections.
}

HAP_RESULT_USE_CHECK
bool HAPPlatformMFiHWAuthIsPoweredOn(HAPPlatformMFiHWAuthRef mfiHWAuth) {
    HAPPrecondition(mfiHWAuth);

    return mfiHWAuth->mcp2221 != NULL;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformMFiHWAuthPowerOn(HAPPlatformMFiHWAuthRef mfiHWAuth) {
    HAPPrecondition(mfiHWAuth);
    HAPPrecondition(!mfiHWAuth->mcp2221);

    mfiHWAuth->mcp2221 = hid_open(kMCP2221_VendorID, kMCP2221_ProductID, NULL);
    if (!mfiHWAuth->mcp2221) {
        HAPLog(&logObject, "hid_open failed.");
        return kHAPError_Unknown;
    }

    return kHAPError_None;
}

void HAPPlatformMFiHWAuthPowerOff(HAPPlatformMFiHWAuthRef mfiHWAuth) {
    HAPPrecondition(mfiHWAuth);
    HAPPrecondition(mfiHWAuth->mcp2221);

    hid_close(mfiHWAuth->mcp2221);
    mfiHWAuth->mcp2221 = NULL;
}

/**
 * HID Report.
 */
typedef struct {
    /**
     * Encoded data for MCP2221.
     */
    uint8_t bytes[kMCP2221_HIDReportSize + 1];
} Report;

/**
 * Performs a HID transaction.
 *
 * @param      mcp2221              MCP2221 instance.
 * @param[in,out] report            On input, HID report to send. On output, received HID report.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the MCP2221.
 */
HAP_RESULT_USE_CHECK
static HAPError MCP2221PerformTransaction(hid_device* mcp2221, Report* report) {
    HAPPrecondition(mcp2221);
    HAPPrecondition(report);

    int numBytes = hid_write(mcp2221, report->bytes, sizeof report->bytes);
    if (numBytes != sizeof report->bytes) {
        HAPLog(&logObject, "hid_write failed: %d", numBytes);
        return kHAPError_Unknown;
    }
    numBytes = hid_read(mcp2221, &report->bytes[1], sizeof report->bytes - 1);
    if (numBytes != sizeof report->bytes - 1) {
        HAPLog(&logObject, "hid_read failed: %d", numBytes);
        return kHAPError_Unknown;
    }
    return kHAPError_None;
}

/**
 * Invokes the `Status/Set Parameters` procedure on the MCP2221.
 *
 * @param      mcp2221              MCP2221 instance.
 * @param      cancel               Whether or not to cancel current transfer.
 * @param[out] i2cState             I2C state.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the MCP2221.
 */
HAP_RESULT_USE_CHECK
static HAPError MCP2221StatusSetParameters(hid_device* mcp2221, bool cancel, uint8_t* _Nullable i2cState) {
    HAPPrecondition(mcp2221);

    HAPError err;

    // See MCP2221 Specification
    // Section 3.1.1 STATUS/SET PARAMETERS
    Report report = { { 0 } };
    size_t o = 0;
    report.bytes[o++] = kMCP2221_HIDReportNumber;
    report.bytes[o++] = kMCP2221CommandCode_StatusSetParameters;
    report.bytes[o++] = 0; // Don't care
    report.bytes[o++] = (uint8_t)(cancel ? 0x10 : 0);
    report.bytes[o++] = 0; // Don't set I2C/SMBus communication speed
    report.bytes[o++] = 0; // No new system clock divider

    err = MCP2221PerformTransaction(mcp2221, &report);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    struct {
        uint8_t commandCode;
        uint8_t error;
        uint8_t cancelTransferRsp;
        uint8_t i2cState;
    } rsp;

    o = 1;
    rsp.commandCode = report.bytes[o++];
    rsp.error = report.bytes[o++];
    rsp.cancelTransferRsp = report.bytes[o++];
    o++; // New communication speed set state
    o++; // New communication speed
    o++; // Don't care
    o++; // Don't care
    o++; // Don't care
    rsp.i2cState = report.bytes[o++];

    if (rsp.commandCode != kMCP2221CommandCode_StatusSetParameters) {
        HAPLog(&logObject, "%s: Received unexpected response command code (0x%02X).", __func__, rsp.commandCode);
        return kHAPError_Unknown;
    }
    if (rsp.error) {
        HAPLog(&logObject, "%s: Received error response (err = 0x%02X).", __func__, rsp.error);
        return kHAPError_Unknown;
    }
    if (i2cState) {
        *i2cState = rsp.i2cState;
    }
    HAPLog(&logObject,
           "%s: Cancel = %d, Cancel rsp = 0x%02X, I2C state = 0x%02X",
           __func__,
           cancel,
           rsp.cancelTransferRsp,
           rsp.i2cState);
    return kHAPError_None;
}

/**
 * Invokes the `I2C Write Data` procedure on the MCP2221.
 *
 * @param      mcp2221              MCP2221 instance.
 * @param      i2cAddress           7-bit I2C address to write to.
 * @param      bytes                Buffer containing data to write.
 * @param      numBytes             Length of buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the MCP2221.
 * @return kHAPError_Busy           If a temporary occurred while accessing the MCP2221.
 */
HAP_RESULT_USE_CHECK
static HAPError MCP2221I2CWriteData(hid_device* mcp2221, uint8_t i2cAddress, const void* bytes, size_t numBytes) {
    HAPPrecondition(mcp2221);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes <= 60); // If the requested length is more than 60 bytes, more commands are necessary.

    HAPError err;

    // See MCP2221 Specification
    // Section 3.1.5 I2C WRITE DATA
    Report report = { { 0 } };
    size_t o = 0;
    report.bytes[o++] = kMCP2221_HIDReportNumber;
    report.bytes[o++] = kMCP2221CommandCode_I2CWriteData;
    HAPWriteLittleUInt16(&report.bytes[o], numBytes);
    o += 2;
    report.bytes[o++] = (uint8_t)(i2cAddress << 1);
    HAPRawBufferCopyBytes(&report.bytes[o], bytes, numBytes);

    err = MCP2221PerformTransaction(mcp2221, &report);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return kHAPError_Busy;
    }

    struct {
        uint8_t commandCode;
        uint8_t error;
        uint8_t state;
    } rsp;

    o = 1;
    rsp.commandCode = report.bytes[o++];
    rsp.error = report.bytes[o++];
    rsp.state = report.bytes[o++];

    if (rsp.commandCode != kMCP2221CommandCode_I2CWriteData) {
        HAPLog(&logObject, "%s: Received unexpected response command code (0x%02X)", __func__, rsp.commandCode);
        return kHAPError_Unknown;
    }
    if (rsp.error) {
        HAPLog(&logObject,
               "%s: Received error response (err = 0x%02X, state = 0x%02X)",
               __func__,
               rsp.error,
               rsp.state);
        return kHAPError_Unknown;
    }
    HAPLogBufferDebug(&logObject, bytes, numBytes, "> %s: Addr = 0x%02X", __func__, i2cAddress);
    return kHAPError_None;
}

/**
 * Invokes the `I2C Read Data` procedure on the MCP2221.
 *
 * @param      mcp2221              MCP2221 instance.
 * @param      i2cAddress           7-bit I2C address to read from.
 * @param      numBytes             Number of bytes to read.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the MCP2221.
 */
HAP_RESULT_USE_CHECK
static HAPError MCP2221I2CReadData(hid_device* mcp2221, uint8_t i2cAddress, size_t numBytes) {
    HAPPrecondition(mcp2221);
    HAPPrecondition(numBytes);

    HAPError err;

    // See MCP2221 Specification
    // Section 3.1.8 I2C READ DATA
    Report report = { { 0 } };
    size_t o = 0;
    report.bytes[o++] = kMCP2221_HIDReportNumber;
    report.bytes[o++] = kMCP2221CommandCode_I2CReadData;
    HAPWriteLittleUInt16(&report.bytes[o], numBytes);
    o += 2;
    report.bytes[o++] = (uint8_t)(i2cAddress << 1 | 1);

    err = MCP2221PerformTransaction(mcp2221, &report);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    struct {
        uint8_t commandCode;
        uint8_t error;
        uint8_t state;
    } rsp;

    o = 1;
    rsp.commandCode = report.bytes[o++];
    rsp.error = report.bytes[o++];
    rsp.state = report.bytes[o++];

    if (rsp.commandCode != kMCP2221CommandCode_I2CReadData) {
        HAPLog(&logObject, "%s: Received unexpected response command code (0x%02X)", __func__, rsp.commandCode);
        return kHAPError_Unknown;
    }
    if (rsp.error) {
        HAPLog(&logObject,
               "%s: Received error response (err = 0x%02X, state = 0x%02X)",
               __func__,
               rsp.error,
               rsp.state);
        return kHAPError_Unknown;
    }
    HAPLogDebug(&logObject, "%s: Addr = 0x%02X, Data length = %lu", __func__, i2cAddress, (unsigned long) numBytes);
    return kHAPError_None;
}

/**
 * Invokes the `Get I2C Data` procedure on the MCP2221.
 *
 * @param      mcp2221              MCP2221 instance.
 * @param      bytes                Buffer to read data into.
 * @param      maxBytes             Capacity of buffer.
 * @param[out] numBytes             Number of bytes read.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the MCP2221.
 */
HAP_RESULT_USE_CHECK
static HAPError MCP2221GetI2CData(hid_device* mcp2221, void* bytes, size_t maxBytes, size_t* numBytes) {
    HAPPrecondition(mcp2221);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    HAPError err;

    // See MCP2221 Specification
    // Section 3.1.8 I2C READ DATA
    Report report = { { 0 } };
    size_t o = 0;
    report.bytes[o++] = kMCP2221_HIDReportNumber;
    report.bytes[o++] = kMCP2221CommandCode_GetI2CData;

    err = MCP2221PerformTransaction(mcp2221, &report);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    struct {
        uint8_t commandCode;
        uint8_t error;
        uint8_t state;
        uint8_t numBytes;
        const uint8_t* bytes;
    } rsp;

    o = 1;
    rsp.commandCode = report.bytes[o++];
    rsp.error = report.bytes[o++];
    rsp.state = report.bytes[o++];
    rsp.numBytes = report.bytes[o++];
    rsp.bytes = &report.bytes[o];

    if (rsp.commandCode != kMCP2221CommandCode_GetI2CData) {
        HAPLog(&logObject, "%s: Received unexpected response command code (0x%02X).", __func__, rsp.commandCode);
        return kHAPError_Unknown;
    }
    if (rsp.error) {
        HAPLog(&logObject,
               "%s: Received error response (err = 0x%02X, state = 0x%02X).",
               __func__,
               rsp.error,
               rsp.state);
        return kHAPError_Unknown;
    }
    if (rsp.numBytes == 127) {
        HAPLog(&logObject, "%s: Error has occurred (length 127).", __func__);
        return kHAPError_Unknown;
    }
    if (rsp.numBytes > sizeof report.bytes - /* report number */ 1 - /* header */ 4) {
        HAPLog(&logObject, "%s: Buffer overflow (received length %d).", __func__, rsp.numBytes);
        return kHAPError_Unknown;
    }
    if (rsp.numBytes > maxBytes) {
        HAPLog(&logObject, "%s: Received data does not fit into response buffer.", __func__);
        return kHAPError_Unknown;
    }
    HAPRawBufferCopyBytes(bytes, rsp.bytes, rsp.numBytes);
    *numBytes = rsp.numBytes;
    HAPLogBufferDebug(&logObject, rsp.bytes, rsp.numBytes, "< %s", __func__);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformMFiHWAuthWrite(HAPPlatformMFiHWAuthRef mfiHWAuth, const void* bytes, size_t numBytes) {
    HAPPrecondition(mfiHWAuth);
    HAPPrecondition(mfiHWAuth->mcp2221);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes >= 1 && numBytes <= 128);

    HAPError err;

    HAPLogDebug(&logObject, "==================================================================");
    HAPLogDebug(&logObject, "Start write.");

    int repeat = 1000;
    while (--repeat) {
        err = MCP2221I2CWriteData(mfiHWAuth->mcp2221, kMCP2221_I2CAddress, bytes, numBytes);
        if (err) {
            HAPAssert(err == kHAPError_Unknown || err == kHAPError_Busy);
            if (err == kHAPError_Busy && repeat > 101) {
                HAPLogInfo(&logObject, "%s: `I2C Write Data` failed. Retrying.", __func__);
                repeat -= 100;
                (void) usleep(500);
                continue;
            }
            HAPLog(&logObject, "%s: `I2C Write Data` failed.", __func__);
            return kHAPError_Unknown;
        }

        uint8_t i2cState = 0;
        do {
            err = MCP2221StatusSetParameters(mfiHWAuth->mcp2221, /* cancel: */ false, &i2cState);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPLog(&logObject, "%s: `Status/Set Parameters` failed.", __func__);
                return err;
            }
        } while (--repeat && (i2cState == kMCP2221I2CStatus_Writing1 || i2cState == kMCP2221I2CStatus_Writing2 ||
                              i2cState == kMCP2221I2CStatus_InProgress));
        if (!repeat) {
            HAPLog(&logObject, "%s: Retries exceeded (writing).", __func__);
            return kHAPError_Unknown;
        }
        switch (i2cState) {
            case kMCP2221I2CStatus_Nack: {
                // Cancel operation, resetting error condition. Then, try again.
                err = MCP2221StatusSetParameters(mfiHWAuth->mcp2221, /* cancel: */ true, /* i2cState: */ NULL);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    HAPLog(&logObject, "%s: `Status/Set Parameters` failed (cancel).", __func__);
                    return err;
                }
                (void) usleep(500);
            }
                continue;
            case kMCP2221I2CStatus_Idle: {
            } break;
            default: {
                HAPLog(&logObject, "%s: Unexpected I2C state: 0x%02X.", __func__, i2cState);
            }
                return kHAPError_Unknown;
        }
        break;
    }
    if (!repeat) {
        HAPLog(&logObject, "%s: Retries exceeded.", __func__);
        return kHAPError_Unknown;
    }
    HAPLogDebug(&logObject, "Write ok.");
    HAPLogDebug(&logObject, "==================================================================");
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformMFiHWAuthRead(
        HAPPlatformMFiHWAuthRef mfiHWAuth,
        uint8_t registerAddress,
        void* bytes,
        size_t numBytes) {
    HAPPrecondition(mfiHWAuth);
    HAPPrecondition(mfiHWAuth->mcp2221);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes >= 1 && numBytes <= 128);

    HAPError err;

    HAPLogDebug(&logObject, "==================================================================");
    HAPLogDebug(&logObject, "Start read.");

    int repeat = 1000;

    // Send register ID to read.
    while (--repeat) {
        err = MCP2221I2CWriteData(mfiHWAuth->mcp2221, kMCP2221_I2CAddress, &registerAddress, sizeof registerAddress);
        if (err) {
            HAPAssert(err == kHAPError_Unknown || err == kHAPError_Busy);
            if (err == kHAPError_Busy && repeat > 101) {
                HAPLogInfo(&logObject, "%s: `I2C Write Data` failed. Retrying.", __func__);
                repeat -= 100;
                (void) usleep(500);
                continue;
            }
            HAPLog(&logObject, "%s: `I2C Write Data` failed.", __func__);
            return kHAPError_Unknown;
        }

        uint8_t i2cState = 0;
        err = MCP2221StatusSetParameters(mfiHWAuth->mcp2221, /* cancel: */ false, &i2cState);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLog(&logObject, "%s: `Status/Set Parameters` failed.", __func__);
            return err;
        }
        switch (i2cState) {
            case kMCP2221I2CStatus_Nack: {
                // Cancel operation, resetting error condition. Then, try again.
                err = MCP2221StatusSetParameters(mfiHWAuth->mcp2221, /* cancel: */ true, /* i2cState: */ NULL);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    HAPLog(&logObject, "%s: `Status/Set Parameters` failed (cancel).", __func__);
                    return err;
                }
                (void) usleep(500);
            }
                continue;
            case kMCP2221I2CStatus_Idle: {
            } break;
            default: {
                HAPLog(&logObject, "%s: Unexpected I2C state (writing reg ID): 0x%02X.", __func__, i2cState);
            }
                return kHAPError_Unknown;
        }
        break;
    }
    if (!repeat) {
        HAPLog(&logObject, "%s: Retries exceeded.", __func__);
        return kHAPError_Unknown;
    }

    // Send read request.
    while (--repeat) {
        err = MCP2221I2CReadData(mfiHWAuth->mcp2221, kMCP2221_I2CAddress, numBytes);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLog(&logObject, "%s: `I2C Read Data` failed.", __func__);
            return err;
        }

        uint8_t i2cState = 0;
        do {
            err = MCP2221StatusSetParameters(mfiHWAuth->mcp2221, /* cancel: */ false, &i2cState);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPLog(&logObject, "%s: `Status/Set Parameters` failed.", __func__);
                return err;
            }
        } while (--repeat && (i2cState == kMCP2221I2CStatus_Reading1 || i2cState == kMCP2221I2CStatus_Reading2 ||
                              i2cState == kMCP2221I2CStatus_InProgress));
        if (!repeat) {
            HAPLog(&logObject, "%s: Retries exceeded (reading).", __func__);
            return kHAPError_Unknown;
        }
        switch (i2cState) {
            case kMCP2221I2CStatus_Nack: {
                // Cancel operation, resetting error condition. Then, try again.
                err = MCP2221StatusSetParameters(mfiHWAuth->mcp2221, true, NULL);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    HAPLog(&logObject, "%s: `Status/Set Parameters` failed (cancel).", __func__);
                    return kHAPError_Unknown;
                }
                (void) usleep(500);
            }
                continue;
            case kMCP2221I2CStatus_DataReadyPartial:
            case kMCP2221I2CStatus_DataReady: {
            } break;
            default: {
                HAPLog(&logObject, "%s: Unexpected I2C state (starting read): 0x%02X.", __func__, i2cState);
            }
                return kHAPError_Unknown;
        }
        break;
    }
    if (!repeat) {
        HAPLog(&logObject, "%s: Retries exceeded.", __func__);
        return kHAPError_Unknown;
    }

    // Read chunks.
    size_t remaining = numBytes;
    while (remaining) {
        size_t num = 0;
        err = MCP2221GetI2CData(mfiHWAuth->mcp2221, &((uint8_t*) bytes)[numBytes - remaining], remaining, &num);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLog(&logObject, "%s: `Get I2C Data` failed.", __func__);
            return err;
        }
        if (!num) {
            HAPLog(&logObject, "%s: No bytes received.", __func__);
            return kHAPError_Unknown;
        }
        remaining -= num;

        uint8_t i2c_state = 0;
        do {
            err = MCP2221StatusSetParameters(mfiHWAuth->mcp2221, /* cancel: */ false, &i2c_state);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPLog(&logObject, "%s: `Status/Set Parameters` failed.", __func__);
                return err;
            }
        } while (--repeat && (i2c_state == kMCP2221I2CStatus_Reading1 || i2c_state == kMCP2221I2CStatus_Reading2 ||
                              i2c_state == kMCP2221I2CStatus_InProgress));
        if (!repeat) {
            HAPLog(&logObject, "%s: Retries exceeded (getting data).", __func__);
            return kHAPError_Unknown;
        }
        switch (i2c_state) {
            case kMCP2221I2CStatus_Idle: {
                if (remaining) {
                    HAPLog(&logObject, "%s: No more data (remaining = %lu).", __func__, (unsigned long) remaining);
                    return kHAPError_Unknown;
                }
            } break;
            case kMCP2221I2CStatus_DataReadyPartial:
            case kMCP2221I2CStatus_DataReady: {
            } break;
            default: {
                HAPLog(&logObject, "%s: Unexpected I2C state (getting data): 0x%02X.", __func__, i2c_state);
            }
                return kHAPError_Unknown;
        }
    }
    HAPLogDebug(&logObject, "Read ok.");
    HAPLogDebug(&logObject, "==================================================================");
    return kHAPError_None;
}

#endif
