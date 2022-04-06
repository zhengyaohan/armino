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
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

#ifndef HAP_PLATFORM_TCP_STREAM_MANAGER_HELPER_H
#define HAP_PLATFORM_TCP_STREAM_MANAGER_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatform.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Creates a connection to the TCP stream manager's listener.
 *
 * - The TCP stream must be closed using HAPPlatformTCPStreamManagerClientClose after it is no longer used.
 *
 * @param      tcpStreamManager     TCP stream manager.
 * @param[out] tcpStream            TCP stream that has been put into the TCP stream manager's accept queue.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOFResources If not enough resources are available to create the connection.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformTCPStreamManagerConnectToListener(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef* tcpStream);

/**
 * Closes a TCP stream for reading and writing.
 *
 * - /!\ WARNING: The TCP stream must no longer be used by the client after this function returns.
 *
 * @param      tcpStreamManager     TCP stream manager.
 * @param      tcpStream            TCP stream that has been put into the TCP stream manager's accept queue.
 */
void HAPPlatformTCPStreamManagerClientClose(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream);

/**
 * Reads from a TCP stream as client.
 *
 * @param      tcpStreamManager     TCP stream manager from which the stream was accepted.
 * @param      tcpStream            TCP stream.
 * @param[out] bytes                Buffer containing received data.
 * @param      maxBytes             Capacity of buffer.
 * @param[out] numBytes             Number of bytes put into buffer.
 *
 * @return kHAPError_None           If successful.
                                    If numBytes is 0, the peer has closed its side of the connection for writing.
 * @return kHAPError_Unknown        If a non-recoverable error occurred while reading from the TCP stream.
 * @return kHAPError_Busy           If no data is available for reading at the time. Retry later.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformTCPStreamClientRead(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes);

/**
 * Writes to a TCP stream.
 *
 * - Partial writes may occur.
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
HAPError HAPPlatformTCPStreamClientWrite(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream,
        const void* bytes,
        size_t maxBytes,
        size_t* numBytes);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
