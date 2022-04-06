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

#import <Foundation/Foundation.h>
#import <Network/Network.h>

#include "HAPPlatformTCPStreamManager+Init.h"

#define kTcpGetPortMaxRetries 5

static const HAPLogObject tcp_log = { .subsystem = kHAPPlatform_LogSubsystem, .category = "TCPStreamManager" };

@interface Connection : NSObject
@property (nonatomic, retain) nw_connection_t socket;
@property nw_connection_state_t state;
@property HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager;
@property HAPPlatformTCPStreamEvent interests;
@property HAPPlatformTCPStreamEventCallback _Nullable callback;
@property void* _Nullable context;
@property (nonatomic, assign) BOOL didDetectAsyncWriteFailure;
@property (nonatomic, retain) NSData* buffer;
@end

@implementation Connection
- (instancetype)initWithSocket:(nw_connection_t)socket manager:(HAPPlatformTCPStreamManagerRef)manager {
    if (self = [super init]) {
        _socket = socket;
        _state = nw_connection_state_invalid;
        _tcpStreamManager = manager;
        _interests = (HAPPlatformTCPStreamEvent) {
            .hasBytesAvailable = false,
            .hasSpaceAvailable = false,
        };
        _callback = NULL;
        _context = NULL;
        _didDetectAsyncWriteFailure = NO;
        _buffer = [[NSMutableData alloc] init];
    }
    return self;
}
@end

@class TCPStreamManager;

@interface TCPStreamManager : NSObject {
@public
    nw_listener_t listener;
    NSMutableArray<Connection*>* socketsWaitingToBeAccepted;
    NSMutableArray<Connection*>* connections;
    HAPPlatformTCPStreamManagerRef tcpStreamManager;
}
@end

@implementation TCPStreamManager

- (instancetype)init {
    if (self = [super init]) {
        listener = nil;
        socketsWaitingToBeAccepted = nil;
        connections = nil;
        tcpStreamManager = nil;
    }
    return self;
}

- (void)EventCallback:(Connection*)connection incoming:(bool)incoming {
    if (![connections containsObject:connection]) {
        return;
    }

    if (connection.didDetectAsyncWriteFailure) {
        // Force a read of the tcpStream upon detection of an unhandled write failure. A tcpStream
        // will fail a read if there is an unhandled write failure, so this will force HAP to handle the
        // tcpStream error.
        connection.callback(
                connection.tcpStreamManager,
                (HAPPlatformTCPStreamRef)(__bridge void*) connection,
                (HAPPlatformTCPStreamEvent) {
                        .hasBytesAvailable = true,
                        .hasSpaceAvailable = false,
                },
                connection.context);
    } else if (
            (connection.interests.hasSpaceAvailable && !incoming) ||
            (connection.interests.hasBytesAvailable && incoming)) {
        connection.callback(
                connection.tcpStreamManager,
                (HAPPlatformTCPStreamRef)(__bridge void*) connection,
                (HAPPlatformTCPStreamEvent) {
                        .hasBytesAvailable = incoming,
                        .hasSpaceAvailable = !incoming,
                },
                connection.context);
    }
}

- (void)WaitForMoreDataInBackground:(Connection*)connection {
    if (![connections containsObject:connection]) {
        return;
    }
    nw_connection_receive(
            connection.socket,
            1,
            4096,
            ^(dispatch_data_t data, nw_content_context_t context HAP_UNUSED, bool is_complete, nw_error_t error) {
                if (error) {
                    return;
                }
                if (is_complete) {
                    HAPLogDebug(&tcp_log, "Remote closed the TCP stream");
                    connection.state = nw_connection_state_cancelled;
                }
                NSMutableData* newdata = [[NSMutableData alloc] init];
                [newdata appendData:connection.buffer];
                [newdata appendData:(NSData*) data];
                connection.buffer = newdata;
                [self EventCallback:connection incoming:true];
                dispatch_async(dispatch_get_main_queue(), ^{
                    [self WaitForMoreDataInBackground:connection];
                });
            });
}

@end

static struct {
    NSMutableArray<TCPStreamManager*>* tcpStreamManagers;
} state = {
    .tcpStreamManagers = nil,
};

static TCPStreamManager* GetTCPStreamManager(HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager) {
    for (size_t i = 0; i < state.tcpStreamManagers.count; i++) {
        if (state.tcpStreamManagers[i] && state.tcpStreamManagers[i]->tcpStreamManager == tcpStreamManager) {
            return state.tcpStreamManagers[i];
        }
    }
    return nil;
}

void HAPPlatformTCPStreamManagerCreate(
        HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager,
        const HAPPlatformTCPStreamManagerOptions* options HAP_UNUSED) {
    if (state.tcpStreamManagers == nil) {
        state.tcpStreamManagers = [[NSMutableArray alloc] init];
    }
    TCPStreamManager* tcpManager = [[TCPStreamManager alloc] init];
    tcpManager->tcpStreamManager = tcpStreamManager;
    tcpManager->socketsWaitingToBeAccepted = [[NSMutableArray alloc] init];
    tcpManager->connections = [[NSMutableArray alloc] init];
    [state.tcpStreamManagers addObject:tcpManager];
}

void HAPPlatformTCPStreamManagerRelease(HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager) {
    TCPStreamManager* tcpManager = GetTCPStreamManager(tcpStreamManager);
    HAPAssert(tcpManager);

    if (tcpManager->listener) {
        HAPLogDebug(&tcp_log, "%s: releasing tcp stream manager", __func__);
        HAPPlatformTCPStreamManagerCloseListener(tcpManager->tcpStreamManager);
    }
    tcpManager->socketsWaitingToBeAccepted = nil;
    tcpManager->connections = nil;
    [state.tcpStreamManagers removeObject:tcpManager];
}

void HAPPlatformTCPStreamManagerSetTCPUserTimeout(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPTime tcpUserTimeout HAP_UNUSED) {
    HAPPrecondition(tcpStreamManager);
}

HAP_RESULT_USE_CHECK
uint16_t HAPPlatformTCPStreamManagerGetListenerPort(HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager) {
    TCPStreamManager* tcpManager = GetTCPStreamManager(tcpStreamManager);
    HAPAssert(tcpManager);

    HAPAssert(tcpManager->tcpStreamManager);
    HAPAssert(tcpManager->listener);

    // Retry for 250 ms
    uint16_t port = kHAPNetworkPort_Any;
    int numRetries = kTcpGetPortMaxRetries;
    while (numRetries) {
        port = nw_listener_get_port(tcpManager->listener);
        if (port != 0) {
            break;
        }
        (void) usleep(50);
        numRetries--;
    }
    return port;
}

HAP_RESULT_USE_CHECK
bool HAPPlatformTCPStreamManagerIsListenerOpen(HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager) {
    HAPPrecondition(tcpStreamManager);
    TCPStreamManager* tcpManager = GetTCPStreamManager(tcpStreamManager);
    HAPAssert(tcpManager);
    return tcpManager->listener != nil;
}

void HAPPlatformTCPStreamManagerOpenListener(
        HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager,
        HAPPlatformTCPStreamListenerCallback _Nonnull callback,
        void* _Nullable context) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(callback);
    TCPStreamManager* tcpManager = GetTCPStreamManager(tcpStreamManager);
    HAPAssert(tcpManager);

    HAPPrecondition(!(tcpManager->listener));
    HAPPrecondition(tcpManager->socketsWaitingToBeAccepted);

    nw_endpoint_t host = nw_endpoint_create_host("::", "0");
    nw_parameters_t tcp =
            nw_parameters_create_secure_tcp(NW_PARAMETERS_DISABLE_PROTOCOL, NW_PARAMETERS_DEFAULT_CONFIGURATION);
    nw_parameters_set_local_endpoint(tcp, host);
    tcpManager->listener = nw_listener_create(tcp);
    HAPAssert(tcpManager->listener);
    nw_listener_set_queue(tcpManager->listener, dispatch_get_main_queue());
    nw_listener_set_new_connection_handler(tcpManager->listener, ^(nw_connection_t socket) {
        Connection* connection = [[Connection alloc] initWithSocket:socket manager:tcpStreamManager];
        [tcpManager->socketsWaitingToBeAccepted addObject:connection];
        callback(tcpManager->tcpStreamManager, context);
    });
    nw_listener_start(tcpManager->listener);
}

void HAPPlatformTCPStreamManagerCloseListener(HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager) {
    HAPPrecondition(tcpStreamManager);
    TCPStreamManager* tcpManager = GetTCPStreamManager(tcpStreamManager);
    HAPPrecondition(tcpManager->listener);
    HAPAssert(tcpManager);
    HAPLogDebug(&tcp_log, "%s: closing tcp stream manager", __func__);
    nw_listener_cancel(tcpManager->listener);
    tcpManager->listener = nil;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformTCPStreamManagerAcceptTCPStream(
        HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager,
        HAPPlatformTCPStreamRef* _Nonnull stream) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(stream);
    TCPStreamManager* tcpManager = GetTCPStreamManager(tcpStreamManager);
    HAPAssert(tcpManager);
    HAPPrecondition(tcpManager->socketsWaitingToBeAccepted.count > 0);

    Connection* connection = tcpManager->socketsWaitingToBeAccepted.lastObject;
    [tcpManager->socketsWaitingToBeAccepted removeLastObject];
    HAPAssert(![tcpManager->connections containsObject:connection]);
    [tcpManager->connections addObject:connection];
    *stream = (HAPPlatformTCPStreamRef)(__bridge void*) connection;

    nw_connection_set_queue(connection.socket, dispatch_get_main_queue());
    nw_connection_set_state_changed_handler(connection.socket, ^(nw_connection_state_t state, nw_error_t error) {
        connection.state = state;
        if (error) {
            return;
        }
        if (state == nw_connection_state_ready) {
            [tcpManager EventCallback:connection incoming:false];
        }
    });
    [tcpManager WaitForMoreDataInBackground:connection];
    nw_connection_start(connection.socket);

    HAPLogDebug(&tcp_log, "Accept %lx", *stream);

    return kHAPError_None;
}

void HAPPlatformTCPStreamCloseOutput(
        HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStream);

    TCPStreamManager* tcpManager = GetTCPStreamManager(tcpStreamManager);
    HAPAssert(tcpManager);
    HAPLogDebug(&tcp_log, "CloseOutput %lx", tcpStream);

    Connection* connection = (__bridge Connection*) (void*) tcpStream;
    HAPAssert([tcpManager->connections containsObject:connection]);
}

void HAPPlatformTCPStreamClose(
        HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStream);

    TCPStreamManager* tcpManager = GetTCPStreamManager(tcpStreamManager);
    HAPAssert(tcpManager);
    HAPLogDebug(&tcp_log, "Close %lx", tcpStream);

    Connection* connection = (__bridge Connection*) (void*) tcpStream;
    HAPAssert([tcpManager->connections containsObject:connection]);

    nw_connection_send(
            connection.socket, NULL, NW_CONNECTION_FINAL_MESSAGE_CONTEXT, true, ^(nw_error_t _Nullable error) {
                if (error != NULL) {
                    HAPLogDebug(
                            &tcp_log,
                            "[%s]: Error code: [%d], [%s]",
                            __func__,
                            nw_error_get_error_code(error),
                            strerror(nw_error_get_error_code(error)));
                }
            });

    [tcpManager->connections removeObject:connection];
}

void HAPPlatformTCPStreamUpdateInterests(
        HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream,
        HAPPlatformTCPStreamEvent interests,
        HAPPlatformTCPStreamEventCallback _Nullable callback,
        void* _Nullable context) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStream);

    TCPStreamManager* tcpManager = GetTCPStreamManager(tcpStreamManager);
    HAPAssert(tcpManager);
    Connection* connection = (__bridge Connection*) (void*) tcpStream;
    HAPAssert([tcpManager->connections containsObject:connection]);

    connection.interests = interests;
    connection.callback = callback;
    connection.context = context;

    if (interests.hasSpaceAvailable && connection.state == nw_connection_state_ready) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [tcpManager EventCallback:connection incoming:false];
        });
    }
    if (interests.hasBytesAvailable && connection.buffer.length > 0) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [tcpManager EventCallback:connection incoming:true];
        });
    }

    HAPLogDebug(
            &tcp_log,
            "SetInterest %lx: %s%s",
            tcpStream,
            interests.hasBytesAvailable ? "hasBytesAvailable " : "",
            interests.hasSpaceAvailable ? "hasSpaceAvailable " : "");
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformTCPStreamRead(
        HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream __unused,
        void* _Nonnull bytes,
        size_t maxBytes,
        size_t* _Nonnull numBytes) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    TCPStreamManager* tcpManager = GetTCPStreamManager(tcpStreamManager);
    HAPAssert(tcpManager);
    Connection* connection = (__bridge Connection*) (void*) tcpStream;
    HAPAssert([tcpManager->connections containsObject:connection]);

    // If an unhandled write error occurred on this connection, let the calling function know about the error.
    if (connection.didDetectAsyncWriteFailure) {
        HAPLogDebug(&tcp_log, "[%s]: Previous write on this tcp stream resulted in an unrecoverable error.", __func__);
        connection.didDetectAsyncWriteFailure = NO;
        return kHAPError_Unknown;
    }
    NSData* buffer = connection.buffer;
    if (!buffer.length) {
        *numBytes = 0;
        if (connection.state == nw_connection_state_ready || connection.state == nw_connection_state_preparing ||
            connection.state == nw_connection_state_waiting) {
            return kHAPError_Busy;
        }
        return kHAPError_None;
    }

    size_t n = buffer.length;
    if (n > maxBytes) {
        n = maxBytes;
    }
    HAPRawBufferCopyBytes(bytes, buffer.bytes, *numBytes = n);
    HAPAssert(n <= buffer.length);
    connection.buffer = [buffer subdataWithRange:NSMakeRange(n, buffer.length - n)];

    // HAPLogBufferDebug(&tcp_log, bytes, n, "[%s]: Read %lx", __func__, tcpStream);

    return kHAPError_None;
}

/**
 *  Writes to a TCP stream.
 * - Partial writes may occur.
 *
 *  At the PAL API level, HAPPlatformTCPStreamWrite() is supposed to behave synchronously, and return after
 *  the write to the tcpStream has succeeded (or has failed). However, the tcpStream write for Darwin behaves
 *  asynchronously, as it uses nw_connection_send(). A completion callback is passed into
 *  nw_connection_send(), which is invoked when the data has been sent, or an error has occurred.
 *
 *  We do the following as a workaround for behaving asynchronously in a synchronous context.
 *  HAPPlatformTCPStreamWrite() and HAPPlatformTCPStreamRead(), before performing any writes or reads to the
 *  tcpStream, check to see if the connection associated with this tcpStream has had an error previously.
 *  - If there was an error, we return an error now, so that the correct error handing can be invoked for an
 *  erroneous write or read (which is closing the tcpStream, among other things).
 *  - If there is no error, it proceeds with the write or read.
 *
 *  In HAPPlatformTCPStreamWrite(), normal behavior involves calling nw_connection_send(), and we flag any
 *  errors in the completion callback. If there is an error, we rely on two things:
 *  1. tcpManager's EventCallback() is called as the last line in the completion callback. In this function,
 *  we force a read of the tcpStream upon detection of a connection error. This will force the tcpStream to
 *  close.
 *  2. Even if this forced read was removed from tcpManager's EventCallback(), any subsequent write or read to
 *  the stream will cause it to return an error and close. If no activity occurs on the tcpStream as the
 *  result of the error, the TCP progression timer will expire, and kill the tcpStream.
 *
 * @param      tcpStreamManager     TCP stream manager from which the stream was accepted.
 * @param      tcpStream            TCP stream.
 * @param      bytes                Buffer containing data to send.
 * @param      maxBytes             Length of buffer.
 * @param[out] numBytes             Number of bytes that have been written.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If a non-recoverable error occurred while writing to the TCP stream.
 * @return kHAPError_Busy           If no space is available for writing at the time. Retry later.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformTCPStreamWrite(
        HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream,
        const void* _Nonnull bytes,
        size_t maxBytes,
        size_t* _Nonnull numBytes) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    TCPStreamManager* tcpManager = GetTCPStreamManager(tcpStreamManager);
    HAPAssert(tcpManager);

    Connection* connection = (__bridge Connection*) (void*) tcpStream;
    HAPAssert([tcpManager->connections containsObject:connection]);
    if (connection == NULL) {
        HAPLogDebug(&tcp_log, "[%s]: Invalid connection", __func__);
        return kHAPError_Unknown;
    }

    // If an unhandled write error occurred on this connection, let the calling function know about the error.
    if (connection.didDetectAsyncWriteFailure) {
        HAPLogDebug(&tcp_log, "[%s]: Previous write on this tcp stream resulted in an unrecoverable error.", __func__);
        connection.didDetectAsyncWriteFailure = NO;
        return kHAPError_Unknown;
    }

    // HAPLogBufferDebug(&tcp_log, bytes, maxBytes, "[%s]: Write %lx", __func__, tcpStream);

    nw_content_context_t context = nw_content_context_create("data");
    nw_connection_send(
            connection.socket,
            dispatch_data_create(bytes, maxBytes, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT),
            context,
            true,
            ^(nw_error_t error) {
                if (error != NULL) {
                    HAPLogDebug(
                            &tcp_log,
                            "[%s]: Error code: [%d], [%s]",
                            __func__,
                            nw_error_get_error_code(error),
                            strerror(nw_error_get_error_code(error)));
                    // Flag in the connection that an error occurred.
                    connection.didDetectAsyncWriteFailure = YES;
                }
                [tcpManager EventCallback:connection incoming:false];
            });
    *numBytes = maxBytes;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
size_t HAPPlatformTCPStreamGetNumNonAcknowledgedBytes(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStream);

    return 0;
}

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
bool HAPPlatformTCPStreamManagerIsWiFiCurrentTransport(HAPPlatformTCPStreamManagerRef tcpStreamManager HAP_UNUSED) {
    return false;
}
#endif

HAP_RESULT_USE_CHECK
HAPPlatformWiFiCapability
        HAPPlatformTCPStreamManagerGetWiFiCapability(HAPPlatformTCPStreamManagerRef tcpStreamManager HAP_UNUSED) {
    return (HAPPlatformWiFiCapability) { .supports2_4GHz = true, .supports5GHz = true };
}

void HAPPlatformTCPStreamSetPriority(
        HAPPlatformTCPStreamManagerRef tcpStreamManager HAP_UNUSED,
        HAPPlatformTCPStreamRef tcpStream HAP_UNUSED,
        HAPStreamPriority priority HAP_UNUSED) {
    HAPLogDebug(&tcp_log, "%s is not supported on this platform.", __func__);
}
