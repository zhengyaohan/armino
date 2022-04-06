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

#ifndef HAP_RTP_CONTROLLER_INTERNAL_H
#define HAP_RTP_CONTROLLER_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPRTPController.h"
#include "HAPSRTP.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * HAP NTP Time
 * 32.32 fractional time [seconds] since 1900
 */
typedef uint64_t HAPNTPTime;

/**
 * HAP Short NTP
 * 16.16 fractional time [seconds] since 1900
 */
typedef uint32_t HAPShortNTP;

/**
 * HAP timestamp
 * relative time mod 2^32 [1/clockFrequency]
 */
typedef uint32_t HAPTimestamp;

/**
 * HAP RTP Stream
 */
typedef struct _HAPRTPStream {
    /**
     * Configuration
     */
    struct {
        /**
         * Payload type id
         */
        uint32_t payloadType;

        /**
         * Timestamp increment frequency [Hz]
         */
        uint32_t clockFrequency;

        /**
         * Clock factor
         * 2^64 * clockFrequency / 10^9
         */
        uint64_t clockFactor;

        /**
         * Local synchronization source
         */
        uint32_t localSSRC;

        /**
         * Remote synchronization source
         */
        uint32_t remoteSSRC;

        /**
         * Maximum RTP packet MTU [bytes]
         */
        uint32_t maxPacketMTU;

        /**
         * RTP packet overhead [bytes]
         */
        uint32_t packetOverhead;

        /**
         * RTCP reporting interval [ms]
         */
        uint32_t rtcpInterval;

        /**
         * TODO: MAGIC NUMBER
         * SDES packet CNAME string
         */
        char sdesCNAMEString[32];

        /**
         * SDES packet CNAME string length
         */
        uint32_t sdesCNAMELength;

        /**
         * Payload type
         */
        HAPRTPEncodeType encodeType;
    } config;

    /**
     * Sender state
     */
    struct {
        /**
         * Actual payload bytes
         */
        const uint8_t* _Nullable payloadBytes;

        /**
         * Actual payload length
         */
        uint32_t numPayloadBytes;

        /**
         * Actual timestamp
         */
        HAPTimestamp timestamp;

        /**
         * Actual fragmentation offset
         */
        int32_t fragmentationOffset;

        /**
         * Index of first packet (16 bit random sequence number)
         */
        uint32_t baseIndex;

        /**
         * Timestamp at start of the stream (random)
         */
        HAPTimestamp baseTimestamp;

        /**
         * Time at start of the stream
         */
        HAPEpochTime baseTime;

        /**
         * Total RTP packets sent
         */
        uint32_t totalPackets;

        /**
         * Payload bytes sent in all packets
         */
        uint32_t totalBytes;

        /**
         * RTP packets sent at last RR sent
         */
        uint32_t packetsAtLastRR;

        /**
         * RTP packets sent at next to last RR sent
         */
        uint32_t packetsAt2LastRR;

        /**
         * Actual RTCP packet index
         */
        uint32_t rtcpIndex;

        /**
         * CVO ID
         */
        uint8_t cvoId;

        /**
         * CVO information byte
         */
        uint8_t cvoInfo;

        /**
         * Previous CVO information byte
         */
        uint8_t prevCvoInfo;

        /**
         * Key frame request state
         */
        volatile uint32_t keyFrame;

        /**
         * Time of the last key frame sent
         */
        volatile HAPEpochTime keyFrameTime;

        /**
         * Time of the last RTCP packet sent
         */
        HAPEpochTime lastRTCPTime;
    } sender;

    /**
     * Receiver state
     */
    struct {
        /**
         * Actual payload bytes
         */
        uint8_t* _Nullable payloadBytes;

        /**
         * Actual payload length
         */
        uint32_t numPayloadBytes;

        /**
         * Actual timestamp
         */
        HAPTimestamp timestamp;

        /**
         * Index (ROC||Seq) of last packet received
         */
        uint32_t lastIndex;

        /**
         * Index (ROC||Seq) of first packet received
         */
        uint32_t firstIndex;

        /**
         * Highest index received
         */
        uint32_t highIndex;

        /**
         * Receiver reference timestamp
         */
        HAPTimestamp referenceTimestamp;

        /**
         * Receiver reference time [ns]
         */
        HAPTimeNS referenceHAPTimeNS;

        /**
         * Total RTP packets received
         */
        uint32_t totalPackets;

        /**
         * Received RTP packets at last RR sent
         */
        uint32_t packetsAtLastRR;

        /**
         * Expected RTP packets at last RR sent
         */
        uint32_t expectedPacketsAtLastRR;

        /**
         * NTP time of last SR received
         */
        HAPShortNTP lastSRTime;

        /**
         * NTP timestamp of last SR received
         */
        HAPNTPTime lastSRNTPStamp;

        /**
         * Timestamp of last SR received
         */
        HAPTimestamp lastSRTimestamp;

        /**
         * Transit time of last RTP packet received
         */
        HAPTimestamp lastTransitTime;

        /**
         * Highest RTCP packet index received
         */
        uint32_t maxRtcpIndex;

        /**
         * Map of RTCP packet indices received
         */
        uint32_t replayMap;

        /**
         * Variance of RTP packet arrival time * 16
         */
        HAPTimestamp scaledJitter;

        /**
         * Round trip time estimated from receiver report
         */
        HAPShortNTP roundTripTime;

        /**
         * Time of last packet received ([s] since 1970).
         */
        uint32_t lastPacketTime;
    } receiver;

    /**
     * Control state
     */
    struct {
        /**
         * Last FIR packet sequence number generated
         */
        uint32_t lastFIRSeqNum;

        /**
         * Last TSTR packet sequence number received
         */
        uint32_t lastTSTRSeqNum;

        /**
         * Actual video bit rate
         */
        uint32_t actualBitRate;

        /**
         * Requested new bit rate
         */
        uint32_t newBitRate;

        /**
         * New TMMBN entry (-1: invalid)
         */
        uint32_t tmmbnEntry;

        /**
         * New TSTN entry (-1: invalid)
         */
        uint32_t tstnEntry;
    } control;

    /**
     * Buffer
     */
    struct {
        /**
         * SPS packet buffer
         */
        uint8_t spsBytes[128];

        /**
         * Length of buffered SPS
         */
        uint32_t numSpsBytes;

        /**
         * PPS packet buffer
         */
        uint8_t ppsBytes[128];

        /**
         * Length of buffered PPS
         */
        uint32_t numPpsBytes;
    } buffer;

    /**
     * SRTP configuration
     */
    struct {
        /**
         * SRTP input crypto context
         */
        HAPSRTPContext inputContext;

        /**
         * SRTP output crypto context
         */
        HAPSRTPContext outputContext;
    } srtp;

    /**
     * SRTCP Crypto configuration
     */
    struct {
        /**
         * SRTCP input crypto context
         */
        HAPSRTPContext inputContext;

        /**
         * SRTCP output crypto context
         */
        HAPSRTPContext outputContext;
    } srtcp;
} HAPRTPStream;
HAP_STATIC_ASSERT(sizeof(HAPRTPStream) >= sizeof(HAPRTPStream), HAPRTPStream);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
