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

#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

#include "HAPAccessoryServer+Internal.h"
#include "HAPCharacteristicTypes+TLV.h"
#include "HAPCharacteristicTypes.h"
#include "HAPLogSubsystem.h"
#include "HAPServiceTypes.h"
#include "HAPTLV+Internal.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "RequestHandlers" };

/**
 * Maximum number of bytes for network name including terminating null
 */
#define NETWORK_NAME_MAX_NUM_BYTES 17

/**
 * Extended PAN ID length in bytes
 */
#define EXT_PAN_ID_LENGTH 8

/**
 * Bit mask for all capabilities bits in Node Capabilities characteristic
 */
#define CAPABILITIES_MASK 0x1F

/**
 * Delay in milliseconds from Thread Stop command to actual stopping of Thread stack.
 *
 * The delay is relevant to the time it takes to send a response to the received request.
 */
#define THREAD_STOP_DELAY 1000

/**
 * Attach timeout in milliseconds.
 *
 * This is the timeout duration within which network attach should succeed.
 * Otherwise, the network attach will be cancelled.
 */
#define THREAD_ATTACH_TIMEOUT 65000

HAP_RESULT_USE_CHECK
HAPError HAPHandleThreadManagementNodeCapabilitiesRead(
        HAPAccessoryServer* server,
        const HAPUInt16CharacteristicReadRequest* request HAP_UNUSED,
        uint16_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(value);

    *value = 0;
    if (server->transports.thread) {
        *value = (uint16_t)(server->thread.deviceParameters.deviceType & CAPABILITIES_MASK);
    }

    return kHAPError_None;
}

/**
 * Converts HAPPlatformThreadDeviceRole into Thread Role characteristic value.
 *
 * @param  role  HAPPlatformThreadDeviceRole value
 * @return Thread Role characteristic value.
 */
static uint16_t PlatformThreadDeviceRoleToCharacteristicValue(HAPPlatformThreadDeviceRole role) {
    switch (role) {
        case kHAPPlatformThreadDeviceRole_Disabled:
            return 0x0001;
        case kHAPPlatformThreadDeviceRole_Detached:
            return 0x0002;
        case kHAPPlatformThreadDeviceRole_Joining:
            return 0x0004;
        case kHAPPlatformThreadDeviceRole_Child:
            return 0x0008;
        case kHAPPlatformThreadDeviceRole_Router:
            return 0x0010;
        case kHAPPlatformThreadDeviceRole_Leader:
            return 0x0020;
        case kHAPPlatformThreadDeviceRole_BR:
            return 0x0040;
    }
    // Unknown value
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleThreadManagementStatusRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt16CharacteristicReadRequest* request HAP_UNUSED,
        uint16_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(value);

    HAPPlatformThreadDeviceRole role;

    HAPError err = HAPPlatformThreadGetRole(&role);
    HAPAssert(!err);
    *value = PlatformThreadDeviceRoleToCharacteristicValue(role);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleThreadManagementCurrentTransportRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(request);
    HAPPrecondition(value);

    *value = (request->transportType == kHAPTransportType_Thread);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleThreadManagementControlRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(responseWriter);

    HAPError err = kHAPError_None;

    if (server->thread.readParametersIsRequested) {
        HAPAssert(server->transports.thread);

        server->thread.readParametersIsRequested = false;

        HAPCharacteristicValue_ThreadManagementControl controlValue;
        HAPRawBufferZero(&controlValue, sizeof controlValue);

        HAPPlatformThreadNetworkParameters threadParams;
        HAPPlatformThreadNetworkParameters* paramPtr = &threadParams;

        if (HAPPlatformThreadIsCommissioned()) {
            err = HAPPlatformThreadGetNetworkParameters(&threadParams);

            if (err != kHAPError_None) {
                return err;
            }
        } else if (server->thread.storage->networkJoinState.parametersAreSet) {
            paramPtr = &server->thread.storage->networkJoinState.parameters;
        } else {
            return kHAPError_None;
        }

        controlValue.networkCredentials.channel = paramPtr->channel;
        controlValue.networkCredentials.panId = paramPtr->panId;

        controlValue.networkCredentialsIsSet = true;
        controlValue.networkCredentials.networkName = paramPtr->networkName;
        controlValue.networkCredentials.extPanId.bytes = paramPtr->extPanId;
        controlValue.networkCredentials.extPanId.numBytes = sizeof paramPtr->extPanId;

        err = HAPTLVWriterEncode(responseWriter, &kHAPCharacteristicTLVFormat_ThreadManagementControl, &controlValue);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "TLV writer failed to encode Thread Control");
        }
        return err;
    }
    return kHAPError_None;
}

/**
 * Stop reattach timer if it is running
 *
 * @param server   accessory server
 */
static void StopReattachTimer(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->thread.storage);

    server->thread.storage->networkJoinState.shouldReattach = false;
    if (server->thread.storage->networkJoinState.reattachTimer) {
        // Cancel ongoing reattach timer
        HAPPlatformTimerDeregister(server->thread.storage->networkJoinState.reattachTimer);
        server->thread.storage->networkJoinState.reattachTimer = 0;
    }
}

/**
 * Timer handler to clear Thread parameters after delay
 */
static void ClearThreadParametersAfterDelay(HAPPlatformTimerRef timer_ HAP_UNUSED, void* _Nullable server_) {
    HAPPrecondition(server_);
    HAPAccessoryServer* server = server_;

    HAPError err = HAPPlatformThreadClearParameters(server);
    HAPAssert(!err);

    // Stop reattach timer if any
    StopReattachTimer(server);

    if (server->thread.isTransportRunning) {
        // Stop Thread transport
        if (server->transports.ble) {
            if (server->ble.isTransportRunning && !server->ble.isTransportStopping) {
                // BLE is already running. Keep it as is and don't stop.
                server->ble.shouldStopTransportWhenDisconnected = false;
            } else {
                // Start BLE transport after stopping Thread transport
                server->ble.shouldStartTransport = true;
            }
        }
        HAPAccessoryServerStopThreadTransport(server);
    }

    if (server->transports.ble) {
        // Start BLE
        HAPAccessoryServerStartBLETransport(server);
    }
}

/**
 * Commissions thread node and trigger Thread startup.
 *
 * @param server         accessory server
 * @param networkName    network name
 * @param channel        channel number
 * @param panId          PAN ID
 * @param extPanId       Ext PAN ID
 * @param masterKey      master key of the network
 * @param attachTimeout  attachment timeout in milliseconds
 * @param formingAllowed network forming is allowed or not
 */
HAP_RESULT_USE_CHECK
static HAPError CommissionThread(
        HAPAccessoryServer* server,
        const char* networkName,
        uint16_t channel,
        uint16_t panId,
        const void* extPanId,
        const void* masterKey,
        uint32_t attachTimeout,
        bool formingAllowed) {
    HAPPrecondition(server);
    HAPPrecondition(networkName);
    HAPPrecondition(extPanId);
    HAPPrecondition(masterKey);

    HAPAssert(server->thread.storage);

    // If multiprotocol library is used, the commissing can start immediately.
    // While multiprotocol library is not in use, platform for the other stack (BLE)
    // must be brought down first.

    server->thread.storage->networkJoinState.formingAllowed = formingAllowed;
    server->thread.storage->networkJoinState.attachTimeout = attachTimeout;
    server->thread.storage->networkJoinState.parameters.channel = channel;
    server->thread.storage->networkJoinState.parameters.panId = panId;
    HAPRawBufferCopyBytes(
            server->thread.storage->networkJoinState.parameters.extPanId,
            extPanId,
            sizeof server->thread.storage->networkJoinState.parameters.extPanId);
    HAPRawBufferCopyBytes(
            server->thread.storage->networkJoinState.parameters.masterKey,
            masterKey,
            sizeof server->thread.storage->networkJoinState.parameters.masterKey);
    size_t networkNameLength = HAPStringGetNumBytes(networkName) + 1;
    if (networkNameLength > sizeof server->thread.storage->networkJoinState.parameters.networkName) {
        HAPLog(&kHAPLog_Default, "Thread network name too long \"%s\"", networkName);
        return kHAPError_InvalidData;
    }
    HAPRawBufferCopyBytes(
            server->thread.storage->networkJoinState.parameters.networkName, networkName, networkNameLength);
    server->thread.storage->networkJoinState.parametersAreSet = true;
    server->thread.storage->networkJoinState.joinerRequested = false;

    // Thread joining must be reattempted if joining fails with a fresh reattach attempt count
    server->thread.storage->networkJoinState.reattachCount = 0;

    // Start Thread transport
    HAPAccessoryServerStartThreadTransport(server);

    return kHAPError_None;
}

/**
 * Initiates Thread Joiner
 *
 * @param server        accessory server
 * @param attachTimeout attach timeout in milliseconds
 */
void InitiateJoiner(HAPAccessoryServer* server, uint32_t attachTimeout) {
    HAPPrecondition(server);

    HAPAssert(server->transports.thread);
    HAPAssert(server->thread.storage);

    // If multiprotocol library is used, joiner can start immediately.
    // While multiprotocol library is not in use, platform for the other stack (BLE)
    // must be brought down first.

    server->thread.storage->networkJoinState.attachTimeout = attachTimeout;
    server->thread.storage->networkJoinState.joinerRequested = true;
    server->thread.storage->networkJoinState.parametersAreSet = false;

    // Stop reattach timer if any
    StopReattachTimer(server);

    // Trigger switching over to Thread
    HAPAccessoryServerStartThreadTransport(server);
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleThreadManagementControlWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_ThreadManagementControl));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_ThreadManagement));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(requestReader);

    HAPCharacteristicValue_ThreadManagementControl controlValue;
    HAPError err =
            HAPTLVReaderDecode(requestReader, &kHAPCharacteristicTLVFormat_ThreadManagementControl, &controlValue);
    if (err || !controlValue.operationTypeIsSet) {
        HAPAssert(err == kHAPError_InvalidData || !controlValue.operationTypeIsSet);
        return kHAPError_InvalidData;
    }

    server->thread.readParametersIsRequested = false;

    switch (controlValue.operationType) {
        case kHAPCharacteristicValue_ThreadManagementControl_OperationType_SetThreadParameters: {
            if (!controlValue.networkCredentialsIsSet) {
                return kHAPError_InvalidData;
            }
            if (request->session->transportType == kHAPTransportType_Thread) {
                HAPLogError(&logObject, "Unable to set Thread Parameters while on thread");
                return kHAPError_InvalidState;
            }

            // Thread management service must have been added only when thread transport is available.
            HAPAssert(server->transports.thread);

            // Schedule thread params to be cleared when we stop Thread.
            err = HAPPlatformThreadClearParameters(server);
            HAPAssert(!err);

            if (server->thread.isTransportRunning) {
                if (server->thread.isTransportStopping) {
                    // Thread transport should be brought up in case it is stopping now.
                    server->thread.shouldStartTransport = true;
                    server->ble.shouldStartTransport = false;
                } else {
                    // Stop reattach timer if any
                    StopReattachTimer(server);
                    // Stop Thread.
                    HAPAccessoryServerStopThreadTransport(server);
                    // We should now be in a position to accept the CommissionThread call.
                }
            }

            err = CommissionThread(
                    server,
                    controlValue.networkCredentials.networkName,
                    controlValue.networkCredentials.channel,
                    controlValue.networkCredentials.panId,
                    controlValue.networkCredentials.extPanId.bytes,
                    controlValue.networkCredentials.masterKey.bytes,
                    THREAD_ATTACH_TIMEOUT,
                    controlValue.formingAllowedIsSet ? (controlValue.formingAllowed != 0) : false);
            break;
        }
        case kHAPCharacteristicValue_ThreadManagementControl_OperationType_ClearThreadParameters: {
            if (server->transports.thread && server->thread.isTransportRunning) {
                // Thread is running.
                // Delay clearing credentials for some time to allow reading response.
                HAPPlatformTimerRef delayTimer;
                err = HAPPlatformTimerRegister(
                        &delayTimer,
                        HAPPlatformClockGetCurrent() + THREAD_STOP_DELAY,
                        ClearThreadParametersAfterDelay,
                        server);
                HAPAssert(!err);
            } else {
                // Thread is not running.
                // Clear parameters immediately.
                ClearThreadParametersAfterDelay(0, server);
            }
            break;
        }
        case kHAPCharacteristicValue_ThreadManagementControl_OperationType_ReadThreadParameters: {
            if (!(server->transports.thread && server->thread.storage)) {
                HAPLogError(&logObject, "Thread Control Read ignored");
                return kHAPError_InvalidState;
            }
            server->thread.readParametersIsRequested = true;
            break;
        }
        case kHAPCharacteristicValue_ThreadManagementControl_OperationType_InitiateThreadJoiner: {
            if (server->transports.thread && server->thread.isTransportRunning) {
                HAPLogError(&logObject, "Thread Control Initiate Joiner ignored");
                return kHAPError_InvalidState;
            }
            InitiateJoiner(server, THREAD_ATTACH_TIMEOUT);
            break;
        }
        default: {
            return kHAPError_InvalidData;
        }
    }

    return err;
}

#endif
