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

#include "HAPRTPController.h"
#include "HAPLogSubsystem.h"
#include "HAPRTPController+Internal.h"
#include "HAPSRTP.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "RTPController" };

const HAPLogObject kHAPRTPController_PacketLog = { .subsystem = kHAP_LogSubsystem, .category = "RTPController" };

/**
 * RTP Package Format
 *
 * Source: RFC 3550 - https://tools.ietf.org/html/rfc3550#section-5.1
 *
 *             0                   1                   2                   3
 *             0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *             +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *             |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 *             +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *             |                           timestamp                           |
 *             +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *             |           synchronization source (SSRC) identifier            |
 *             +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *             |            contributing source (CSRC) identifiers             |
 *             |                             ....                              |
 *             +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  Name                        Length                                      Byte
 *  ----------------------------------------------------------------------------
 *  Version (V)                 2 bits                                      00
 *  Padding (P)                 1 bit
 *  Extensions (X)              1 bit
 *  CSRC count (CC)             4 bits
 *  Marker (M)                  1 bit                                       01
 *  Payload type (PT)           7 bits
 *  Sequence number             16 bits                                     02
 *  Timestamp                   32 bits                                     04
 *  SSRC                        32 bits                                     08
 *  CSRC list                   0 to 15 items, 32 bits each                 12
 */

/**
 * SRTP Package Format
 *
 * Source: RFC 3711 - https://tools.ietf.org/html/rfc3711#section-3.4
 *
 *             0                   1                   2                   3
 *             0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *             +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+<+
 *             |V=2|P|    RC   |   PT=SR or RR   |             length          | |
 *             +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |
 *             |                         SSRC of sender                        | |
 *           +>+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+ |
 *           | ~                          sender info                          ~ |
 *           | +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |
 *           | ~                         report block 1                        ~ |
 *           | +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |
 *           | ~                         report block 2                        ~ |
 *           | +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |
 *           | ~                              ...                              ~ |
 *           | +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |
 *           | |V=2|P|    SC   |  PT=SDES=202  |             length            | |
 *           | +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+ |
 *           | |                          SSRC/CSRC_1                          | |
 *           | +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |
 *           | ~                           SDES items                          ~ |
 *           | +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+ |
 *           | ~                              ...                              ~ |
 *           +>+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+ |
 *           | |E|                         SRTCP index                         | |
 *           | +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+<+
 *           | ~                     SRTCP MKI (OPTIONAL)                      ~ |
 *           | +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |
 *           | :                     authentication tag                        : |
 *           | +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |
 *           |                                                                   |
 *           +-- Encrypted Portion                    Authenticated Portion -----+
 *
 *  Name                        Length
 *  ----------------------------------------------------------------------------
 *  E-flag                      1 bit (required)
 *  SRTCP index                 31 bits (required)
 *  Authentication tag          Configurable length (required)
 *  MKI                         Configurable length (optional)
 */

/**
 * RTP constants
 */
#define kRTPVersion       (2U)      // RTP version
#define kRTPHeaderSize    (12)      // RTP header size
#define kCVOHeaderSize    (8)       // RTP CVO header size
#define kRTCPHeaderSize   (8)       // RTCP header size
#define kOneByteHeaderExt (0xBEDEU) // one byte header extension pattern

/**
 * RTCP packet types
 * RFC3550, Section 12.1 https://tools.ietf.org/html/rfc3550#section-12.1
 */
#define kPacketSR    (200) // Sender report packet type
#define kPacketRR    (201) // Receiver report packet type
#define kPacketSDES  (202) // Source description packet type
#define kPacketRTPFB (205) // RTPFB packet type (RFC 4585, Section 6.1)
#define kPacketPSFB  (206) // PSFB packet type  (RFC 4585, Section 6.1)

/**
 * RTP SDES Types
 * RFC3550, Section 12.2 https://tools.ietf.org/html/rfc3550#section-12.2
 */
#define kSDES_CNAME (1) // CNAME subtype

/**
 * RTP Feedback Messages
 * RFC5104, Section 4.2 https://tools.ietf.org/html/rfc5104#section-4.2
 *
 * Message format (https://tools.ietf.org/html/rfc5104#section-4.2.1.1):
 *
 *              0                   1                   2                   3
 *              0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *             +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *             |                              SSRC                             |
 *             +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *             | MxTBR Exp |  MxTBR Mantissa                 |Measured Overhead|
 *             +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  Name                        Length
 *  ----------------------------------------------------------------------------
 *  SSRC                        32 bits
 *  MxTBR Exp                   6 bits      Unsigned integer [0..64], exponential scaling for mantissa
 *  MxTBR Mantissa              17 bits     Unsigned integer, maximum total media bit rate
 *  Measured overhead           9 bits      Unsigned integer [0..511]
 */
#define kRTPFB_TMMBR (3) // TMMBR subtype
#define kRTPFB_TMMBN (4) // TMMBN subtype

/**
 * Payload-Specific Feedback Messages
 * RFC 4585, Section 6.3 https://tools.ietf.org/html/rfc4585#section-6.3
 */
#define kPSFB_PLI  (1) // Picture Loss Indication (PLI) subtype
#define kPSFB_FIR  (4) // Full Intra Request (FIR) subtype (RFC 5104, Section 4.3)
#define kPSFB_TSTR (5) // Temporal-Spatial Trade-off Request (TSTR) subtype (RFC 5104, Section 4.3)
#define kPSFB_TSTN (6) // Temporal-Spatial Trade-off Notification subtype (RFC 5104, Section 4.3)

/**
 * H264 constants
 */
#define kH264TypeIDR (5) // H264 IDR type
#define kH264TypeSPS (7) // H264 SPS type
#define kH264TypePPS (8) // H264 PPS type

/**
 * H264 Single-Time Aggregation Packet type
 * RFC 6184, Section 5.2 https://tools.ietf.org/html/rfc6184#section-5.2
 */
#define kH264TypeStapA (24) // H264 STAP-A type

/**
 * H264 Fragmentation Units
 * RFC 6184, Section 5.8 https://tools.ietf.org/html/rfc6184#section-5.8
 */
#define kH264TypeFuA (28U) // H264 FU-A type

/**
 * Key frame request
 */
#define kKeyFrameIdle     (0)
#define kKeyFrameGenerate (1)
#define kKeyFrameRequest  (2)

/**
 * SRTP constants
 */
#define kSRTPSession_AuthTagSize   (10)
#define kSRTPSession_AES128KeySize (16)
#define kSRTPSession_AES256KeySize (32)

/**
 * Other constants
 */
#define kInvalid           ((uint32_t) -1)      // invalid states
#define kSeconds1900To1970 (2208988800)         // seconds from 1900 to 1970
#define k2p32d10p9         (0x4B82FA09B5A52CBA) // 2^64 * (2^32 / 10^9 - 4)
#define kMaxBitRate        (25000000)           // max bitrate (high profile and level 4)
#define kMaxUDPPacketSize  (65536)              // max size of a UDP packet

////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef __int128
#define FixMul64(x, y) ((uint64_t)(((unsigned __int128) (x) * (unsigned __int128) (y)) >> 64))
#else
/**
 * 64 bit fixpoint multiplication.
 *
 * @param      x                    64bit unsigned value.
 * @param      y                    64bit unsigned value.
 *
 * @return                          x*y
 */
static uint64_t FixMul64(uint64_t x, uint64_t y) {
    uint32_t xl = (uint32_t) x;
    uint32_t xh = (uint32_t)(x >> 32U);
    uint32_t yl = (uint32_t) y;
    uint32_t yh = (uint32_t)(y >> 32U);
    uint64_t p0 = (uint64_t) xl * (uint64_t) yl;
    uint64_t p1 = (uint64_t) xh * (uint64_t) yl;
    uint64_t p2 = (uint64_t) xl * (uint64_t) yh;
    uint64_t p3 = (uint64_t) xh * (uint64_t) yh;
    uint64_t sum = (p0 >> 32U) + (uint32_t) p1 + (uint32_t) p2;
    return p3 + (sum >> 32U) + (p1 >> 32U) + (p2 >> 32U);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Convert epoch time to NTP time.
 *
 * @param      time                 Epoch time.
 *
 * @return                          NTP time.
 */
HAP_RESULT_USE_CHECK
static HAPNTPTime HAPEpochTimeToHAPNTPTime(HAPEpochTime time) {
    time += kSeconds1900To1970 * HAPSecondNS;
    return FixMul64(time, k2p32d10p9) + (time << 2U); // ns * 2^32 / 10^9
}

/**
 * Convert NTP time to HAP time [ns].
 *
 * @param      time                 NTP time.
 *
 * @return                          HAP time [ns].
 */
HAP_RESULT_USE_CHECK
static HAPTimeNS HAPNTPTimeToNS(HAPNTPTime time) {
    uint64_t high = (time >> 32U) * HAPSecondNS;
    uint32_t low = (uint32_t)((uint32_t) time * HAPSecondNS >> 32U);
    return high + low; // time * 10^9 / 2^32
}

/**
 * Convert HAP time [ns] to timestamp.
 *
 * @param      stream               HAP RTP stream.
 * @param      ns                   HAP time.
 *
 * @return                          Timestamp.
 */
HAP_RESULT_USE_CHECK
static HAPTimestamp NSToTimestamp(HAPRTPStream* stream, HAPTimeNS ns) {
    uint32_t ts = (uint32_t) FixMul64(ns, stream->config.clockFactor);
    return ts + stream->sender.baseTimestamp;
}

/**
 * Convert epoch time to timestamp.
 *
 * @param      stream               HAP RTP stream.
 * @param      time                 Epoch time.
 *
 * @return                          Timestamp.
 */
HAP_RESULT_USE_CHECK
static uint32_t TimeToTimestamp(HAPRTPStream* stream, HAPEpochTime time) {
    return NSToTimestamp(stream, time - stream->sender.baseTime);
}

/**
 * Convert timestamp to HAP Time [ns].
 *
 * @param      stream               HAP RTP stream.
 * @param      timestamp            Timestamp.
 *
 * @return Converted HAP Time [ns].
 */
HAP_RESULT_USE_CHECK
static HAPTimeNS TimestampToNS(HAPRTPStream* stream, uint32_t timestamp) {
    uint64_t ns = 0;
    uint32_t delta = timestamp - stream->receiver.referenceTimestamp; // [timestamp units]
    switch (stream->config.clockFrequency) {
        case 8000: {
            ns = delta * (HAPMillisecondNS / 8ULL);
            break;
        }
        case 16000: {
            ns = delta * (HAPMillisecondNS / 16ULL);
            break;
        }
        case 24000: {
            ns = delta * (((HAPMillisecondNS << 24U) + 12) / 24) >> 24U;
            break;
        }
        default:
            HAPAssertionFailure();
    }
    ns += stream->receiver.referenceHAPTimeNS;
    if (delta >= stream->config.clockFrequency) {
        // readjust reference
        stream->receiver.referenceTimestamp += stream->config.clockFrequency;
        stream->receiver.referenceHAPTimeNS += HAPSecondNS;
    }
    return ns;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Setup SRTP crypto keys.
 *
 * @param[out] srtpContext          SRTP context to be setup.
 * @param[out] srtcpContext         SRTCP context to be setup.
 * @param      parameters           SRTP parameters.
 * @param      ssrc                 Synchronization source.
 */
static void SetupSRTPKeys(
        HAPSRTPContext* srtpContext,
        HAPSRTPContext* srtcpContext,
        const HAPPlatformCameraSRTPParameters* parameters,
        uint32_t ssrc) {
    const uint8_t *key, *salt;
    uint32_t keySize, tagSize;

    if (parameters->cryptoSuite == kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80) {
        keySize = kSRTPSession_AES128KeySize;
        tagSize = kSRTPSession_AuthTagSize;
        key = parameters->_.AES_CM_128_HMAC_SHA1_80.key;
        salt = parameters->_.AES_CM_128_HMAC_SHA1_80.salt;
    } else if (parameters->cryptoSuite == kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80) {
        keySize = kSRTPSession_AES256KeySize;
        tagSize = kSRTPSession_AuthTagSize;
        key = parameters->_.AES_256_CM_HMAC_SHA1_80.key;
        salt = parameters->_.AES_256_CM_HMAC_SHA1_80.salt;
    } else {
        keySize = 0;
        tagSize = 0;
        key = NULL;
        salt = NULL;
    }
    HAPSRTPSetupContext(srtpContext, srtcpContext, key, keySize, salt, tagSize, ssrc);
}

HAP_RESULT_USE_CHECK
HAPError HAPRTPStreamStart(
        HAPRTPStream* stream,
        const HAPPlatformCameraRTPParameters* rtpParameters,
        HAPRTPEncodeType encodeType,
        uint32_t clockFrequency, // [Hz]
        uint32_t localSSRC,
        HAPEpochTime startTime, // [ns] since 1970
        const char* cnameString,
        const HAPPlatformCameraSRTPParameters* srtpInParameters,
        const HAPPlatformCameraSRTPParameters* srtpOutParameters) {
    HAPPrecondition(stream);
    HAPPrecondition(rtpParameters);
    HAPPrecondition(cnameString);
    HAPPrecondition(srtpInParameters);
    HAPPrecondition(srtpOutParameters);

    uint32_t maxMTU = rtpParameters->maximumMTU;

    HAPRawBufferZero(stream, sizeof *stream);
    HAPPlatformRandomNumberFill(&stream->sender.baseTimestamp, sizeof stream->sender.baseTimestamp);
    HAPPlatformRandomNumberFill(&stream->sender.baseIndex, sizeof stream->sender.baseIndex);
    stream->sender.baseIndex &= 0xFFFFU;
    stream->sender.baseTime = startTime;
    stream->sender.keyFrameTime = startTime;
    stream->sender.lastRTCPTime = startTime;
    stream->receiver.lastPacketTime = (uint32_t)(HAPEpochTimeToHAPNTPTime(startTime) >> 32U); // [s]
    stream->config.rtcpInterval = (uint32_t)(rtpParameters->minimumRTCPInterval * 1000.0F);   // [ms]
    stream->config.encodeType = encodeType;
    stream->config.payloadType = rtpParameters->type;
    stream->config.clockFrequency = clockFrequency;
    stream->config.clockFactor = FixMul64((uint64_t) clockFrequency << 32U, k2p32d10p9) +
                                 ((uint64_t) clockFrequency << 34U); // 2^64 * clockFrequency / 10^9
    stream->config.localSSRC = localSSRC;
    stream->config.remoteSSRC = rtpParameters->ssrc;
    stream->control.actualBitRate = rtpParameters->maximumBitRate * 1000U; // [bit/s]
    stream->control.newBitRate = stream->control.actualBitRate;
    unsigned int i;
    for (i = 0; i < sizeof stream->config.sdesCNAMEString - 1 && cnameString[i]; i++) {
        stream->config.sdesCNAMEString[i] = cnameString[i];
    }
    stream->config.sdesCNAMEString[i] = 0;
    stream->config.sdesCNAMELength = i;
    stream->control.lastFIRSeqNum = kInvalid;  // invalid sequence number
    stream->control.lastTSTRSeqNum = kInvalid; // invalid sequence number
    stream->control.tmmbnEntry = kInvalid;     // invalid TMMBN entry
    stream->control.tstnEntry = kInvalid;      // invalid TSTN entry

    // setup crypto keys
    SetupSRTPKeys(&stream->srtp.inputContext, &stream->srtcp.inputContext, srtpInParameters, rtpParameters->ssrc);
    SetupSRTPKeys(&stream->srtp.outputContext, &stream->srtcp.outputContext, srtpOutParameters, localSSRC);

    // reserve space for RTP header and SRTP overhead
    if (maxMTU == 0) {
        maxMTU = kMaxUDPPacketSize;
    } // dummy MTU for audio packets
    stream->config.maxPacketMTU = maxMTU;
    stream->config.packetOverhead = kRTPHeaderSize + stream->srtp.outputContext.tagSize;

    return kHAPError_None;
}

void HAPRTPStreamReconfigure(
        HAPRTPStream* stream,
        uint32_t bitRate,      // [bit/s]
        uint32_t rtcpInterval) // [ms]
{
    HAPPrecondition(stream);

    stream->control.actualBitRate = bitRate;
    if (rtcpInterval) { // ignore rtcpInterval == 0
        stream->config.rtcpInterval = rtcpInterval;
    }
}

void HAPRTPStreamEnd(HAPRTPStream* stream) {
    HAPPrecondition(stream);

    HAPRawBufferZero(stream, sizeof *stream);
}

void HAPRTPStreamResetDropoutTimer(HAPRTPStream* stream, HAPEpochTime actualTime) {
    HAPPrecondition(stream);

    stream->receiver.lastPacketTime = (uint32_t)(HAPEpochTimeToHAPNTPTime(actualTime) >> 32U); // [s];
}

/**
 * Set the CVO on the HAP RTP stream.
 *
 * @param      stream               HAP RTP stream.
 * @param      cvoID                CVO Id.
 * @param      cvoInformation       CVO Information.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed (cvoId > 14).
 */
HAP_RESULT_USE_CHECK
HAPError HAPRTPStreamSetCVO(HAPRTPStream* stream, uint8_t cvoID, uint8_t cvoInformation) {
    HAPPrecondition(stream);

    if (cvoID > 14) {
        return kHAPError_InvalidData;
    }

    stream->sender.cvoId = cvoID;
    stream->sender.cvoInfo = cvoInformation;
    return kHAPError_None;
}

/**
 * Poll RTCP Packet from stream.
 *
 * @param      stream               HAP RTP stream.
 * @param[out] bytes                Buffer that will be filled with the RTCP packet.
 * @param      maxBytes             Maximum number of bytes that may be filled into the buffer.
 * @param[out] numBytes             Effective number of bytes written to the buffer.
 */
static void PollRTCPPacket(
        HAPRTPStream* stream,
        HAPEpochTime actualTime,
        uint8_t* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(stream);
    HAPPrecondition(maxBytes >= 256); // 104 + sizeof stream->sdesCnameString + 14

    uint8_t* packet = bytes;
    uint16_t size;
    uint32_t length;
    uint16_t sr = 0, rr = 0;

    uint32_t sent = stream->sender.totalPackets;
    uint32_t deltaSent = sent - stream->sender.packetsAt2LastRR; // sent during last 2 RTCP intervals
    uint32_t received = stream->receiver.totalPackets;
    uint32_t deltaReceived = received - stream->receiver.packetsAtLastRR;
    uint32_t expected = stream->receiver.highIndex - stream->receiver.firstIndex + 1;
    uint32_t deltaExpected = expected - stream->receiver.expectedPacketsAtLastRR;
    HAPNTPTime ntpTime = HAPEpochTimeToHAPNTPTime(actualTime);

    stream->sender.lastRTCPTime = actualTime;
    stream->sender.packetsAt2LastRR = stream->sender.packetsAtLastRR;
    stream->sender.packetsAtLastRR = sent;
    stream->receiver.packetsAtLastRR = received;
    stream->receiver.expectedPacketsAtLastRR = expected;

    // See RTP package format above.

    if (deltaSent) {
        sr = 1;
    }
    if (deltaReceived) {
        rr = 1;
    }

    // Report packet header.
    packet[0] = (uint8_t)((kRTPVersion << 6U) + rr);   // P = 0
    packet[1] = (uint8_t)(sr ? kPacketSR : kPacketRR); // SR or RR
    size = (uint16_t)(1 + sr * 5 + rr * 6);
    packet[2] = (uint8_t)(size >> 8U);
    packet[3] = (uint8_t) size;
    HAPWriteBigUInt32(&packet[4], stream->config.localSSRC); // sender SSRC
    packet += kRTCPHeaderSize;

    // Sender report.
    if (sr) {
        HAPWriteBigUInt64(packet, ntpTime); // NTP timestamp
        packet += 8;
        HAPWriteBigUInt32(packet, TimeToTimestamp(stream, actualTime)); // RTP timestamp
        packet += 4;
        HAPWriteBigUInt32(packet, sent); // packet count
        packet += 4;
        HAPWriteBigUInt32(packet, stream->sender.totalBytes); // octet count
        packet += 4;
    }

    // Receiver report.
    if (rr) {
        int32_t lost = (int32_t)(expected - received);
        int32_t deltaLost = (int32_t)(deltaExpected - deltaReceived);
        uint32_t fraction = 0;
        HAPShortNTP lastTime = 0, lastDelay = 0;

        if (lost > 0x7FFFFF) {
            lost = 0x7FFFFF; // saturate to 24 bit
        } else if (lost < -0x800000) {
            lost = -0x800000;
        } else {
            // No saturation needed.
        }

        if (deltaLost > 0 && deltaExpected != 0) {
            fraction = ((uint32_t) deltaLost << 8U) / deltaExpected;
        }
        if (stream->receiver.lastSRNTPStamp != 0) {
            lastTime = (uint32_t)(stream->receiver.lastSRNTPStamp >> 16U);
            lastDelay = (uint32_t)(ntpTime >> 16U) - stream->receiver.lastSRTime;
        }

        HAPWriteBigUInt32(packet, stream->config.remoteSSRC); // source SSRC
        packet += 4;
        HAPWriteBigUInt32(packet, (uint32_t) lost); // cumulative lost (24 bit)
        packet[0] = (uint8_t) fraction;             // fraction lost
        packet += 4;
        HAPWriteBigUInt32(packet, stream->receiver.highIndex); // highest index
        packet += 4;
        HAPWriteBigUInt32(packet, stream->receiver.scaledJitter >> 4U); // jitter
        packet += 4;
        HAPWriteBigUInt32(packet, lastTime); // last SR time
        packet += 4;
        HAPWriteBigUInt32(packet, lastDelay); // last SR delay
        packet += 4;
    }
    HAPAssert((size + 1) * 4 == packet - bytes);

    // SDES packet.
    packet[0] = (kRTPVersion << 6U) + 1; // 1 chunk
    packet[1] = kPacketSDES;             // SDES
    length = stream->config.sdesCNAMELength;
    size = (uint16_t)((length + 10) >> 2U);
    packet[2] = (uint8_t)(size >> 8U);
    packet[3] = (uint8_t) size;
    HAPWriteBigUInt32(&packet[4], stream->config.localSSRC); // sender SSRC
    packet[8] = kSDES_CNAME;                                 // CNAME
    packet[9] = (uint8_t) length;
    packet += 10;

    HAPRawBufferCopyBytes(packet, stream->config.sdesCNAMEString, length);
    packet += length;

    do { // Terminate and align to 32 bit.
        packet[0] = 0;
        packet++;
    } while ((uintptr_t) packet & 3U);

    // TMMBN packet.
    if (stream->control.tmmbnEntry != kInvalid) {
        packet[0] = (kRTPVersion << 6U) + kRTPFB_TMMBN;
        packet[1] = kPacketRTPFB; // RTPFB payload
        size = 4;
        packet[2] = (uint8_t)(size >> 8U);
        packet[3] = (uint8_t) size;
        HAPWriteBigUInt32(&packet[4], stream->config.localSSRC);   // sender SSRC
        HAPWriteBigUInt32(&packet[8], 0U);                         // media SSRC
        HAPWriteBigUInt32(&packet[12], stream->config.remoteSSRC); // owner SSRC
        HAPWriteBigUInt32(&packet[16], stream->control.tmmbnEntry);
        stream->control.tmmbnEntry = kInvalid;
        packet += 20;
    }

    // TSTN packet.
    if (stream->control.tstnEntry != kInvalid) {
        packet[0] = (kRTPVersion << 6U) + kPSFB_TSTN;
        packet[1] = kPacketPSFB; // PSFB payload
        size = 4;
        packet[2] = (uint8_t)(size >> 8U);
        packet[3] = (uint8_t) size;
        HAPWriteBigUInt32(&packet[4], stream->config.localSSRC);   // sender SSRC
        HAPWriteBigUInt32(&packet[8], 0U);                         // media SSRC
        HAPWriteBigUInt32(&packet[12], stream->config.remoteSSRC); // owner SSRC
        HAPWriteBigUInt32(&packet[16], stream->control.tstnEntry);
        stream->control.tstnEntry = kInvalid;
        packet += 20;
    }

    // Log packet before encryption.
    HAPLogBufferDebug(&kHAPRTPController_PacketLog, bytes, (size_t)(packet - bytes), "(%p) >", (const void*) stream);

    // Encryption.
    if (stream->srtcp.outputContext.keySize) {
        uint32_t index = stream->sender.rtcpIndex;
        stream->sender.rtcpIndex = (index + 1) & 0x7FFFFFFFU;
        HAPSRTPEncrypt(
                &stream->srtcp.outputContext,
                bytes + kRTCPHeaderSize, // exclude first header
                bytes + kRTCPHeaderSize,
                0,
                (size_t)(packet - (bytes + kRTCPHeaderSize)),
                index);
        index |= 0x80000000;              // set E flag
        HAPWriteBigUInt32(packet, index); // append index
        packet += 4;
        HAPSRTPAuthenticate(
                &stream->srtcp.outputContext,
                packet, // append tag
                bytes,  // include header and index
                (size_t)(packet - bytes - 4),
                index);
        packet += stream->srtcp.outputContext.tagSize;
    }

    *numBytes = (size_t)(packet - bytes);
}

void HAPRTPStreamCheckFeedback(
        HAPRTPStream* stream,
        HAPEpochTime actualTime,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes,
        uint32_t* _Nullable bitRate,
        bool* _Nullable newKeyFrame,
        uint32_t* _Nullable dropoutTime) {
    HAPPrecondition(stream);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    // Default length.
    *numBytes = 0;

    // Check max bit rate change.
    if (bitRate) {
        if (stream->control.newBitRate != stream->control.actualBitRate) {
            *bitRate = stream->control.newBitRate;
            stream->control.actualBitRate = stream->control.newBitRate;
        } else {
            *bitRate = 0;
        }
    }

    // Check key frame request.
    if (newKeyFrame) {
        if (stream->sender.keyFrame == kKeyFrameRequest) {
            *newKeyFrame = true;
            stream->sender.keyFrame = kKeyFrameGenerate;
        } else {
            *newKeyFrame = false;
        }
    }

    // Check dropout time.
    if (dropoutTime) {
        *dropoutTime = (uint32_t)(HAPEpochTimeToHAPNTPTime(actualTime) >> 32U) - stream->receiver.lastPacketTime; // [s]
    }

    // Check control output.
    HAPTimeNS interval_ns = stream->config.rtcpInterval * HAPMillisecondNS; // [ms] -> [ns]
    if (stream->control.tmmbnEntry != kInvalid ||                           // immediate feedback needed
        actualTime >= stream->sender.lastRTCPTime + interval_ns) {
        // RTCP timeout.
        PollRTCPPacket(stream, actualTime, bytes, maxBytes, numBytes);
    }
}

/**
 * Push H264 payload to HAP RTP stream.
 *
 * @param      stream               HAP RTP stream.
 * @param      bytes                H264 payload.
 * @param      numBytes             H264 payload length.
 * @param[out] numPayloadBytes      The size of the unformatted output.
 * @param      actualTime           Actual time of the payload.
 */
static void PushH264Payload(
        HAPRTPStream* stream,
        const uint8_t* bytes,
        size_t numBytes,
        size_t* numPayloadBytes,
        HAPEpochTime actualTime) // [ns] since 1970
{
    HAPPrecondition(stream);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes > 0);
    HAPPrecondition(numPayloadBytes);

    uint8_t type = (uint8_t)(bytes[0] & 0x1FU); // NAL header byte
    *numPayloadBytes = numBytes;

    if (type == kH264TypeIDR) {
        // Update key frame time.
        stream->sender.keyFrame = kKeyFrameGenerate; // set key frame flag to protect access to keyFrameTime
        stream->sender.keyFrameTime = actualTime;
        stream->sender.keyFrame = kKeyFrameIdle; // reset key frame request state
        // send sps/pps with same timestamp as key frame
        if (stream->buffer.numSpsBytes && stream->buffer.numPpsBytes) {
            stream->sender.fragmentationOffset = -1; // send sps/pps packet first
        }
    } else if (type == kH264TypeSPS && numBytes < sizeof stream->buffer.spsBytes) {
        // Store in SPS buffer.
        HAPRawBufferCopyBytes(stream->buffer.spsBytes, bytes, numBytes);
        stream->buffer.numSpsBytes = (uint32_t) numBytes;
        stream->sender.payloadBytes = NULL;
        *numPayloadBytes = 0;
    } else if (type == kH264TypePPS && numBytes < sizeof stream->buffer.ppsBytes) {
        // Store in SPS buffer.
        HAPRawBufferCopyBytes(stream->buffer.ppsBytes, bytes, numBytes);
        stream->buffer.numPpsBytes = (uint32_t) numBytes;
        stream->sender.payloadBytes = NULL;
        *numPayloadBytes = 0;
    } else {
        // Do nothing.
    }
}

void HAPRTPStreamPushPayload(
        HAPRTPStream* stream,
        const void* bytes,
        size_t numBytes,
        size_t* numPayloadBytes,
        HAPTimeNS sampleTime,    // [ns] from stream start
        HAPEpochTime actualTime) // [ns] since 1970
{
    HAPPrecondition(stream);
    HAPPrecondition(bytes);
    HAPPrecondition(numPayloadBytes);

    const uint8_t* buffer = (const uint8_t*) bytes;

    // Ignore empty packets.
    if (!numBytes) {
        *numPayloadBytes = 0;
        return;
    }

    *numPayloadBytes = numBytes;

    stream->sender.payloadBytes = buffer;
    stream->sender.numPayloadBytes = (uint32_t) numBytes;
    stream->sender.timestamp = NSToTimestamp(stream, sampleTime);
    stream->sender.fragmentationOffset = 0;

    if (stream->config.encodeType == kHAPRTPEncodeType_H264) {
        PushH264Payload(stream, buffer, numBytes, numPayloadBytes, actualTime);
    }
}

/**
 * Poll RTP packet from payload.
 *
 * @param      stream               HAP RTP stream.
 * @param      payloadBytes         Payload to include in RTP packet.
 * @param      numBytes             Effective payload length.
 * @param[out] packetBytes          Buffer that will be filled with RTP packet.
 * @param      maxBytes             Maximum number of bytes that may be filled into the buffer.
 * @param[out] numPacketBytes       Effective number of bytes written to the buffer.
 * @param      fragmentHeader       Fragment header.
 * @param      cvoId                CVO Id.
 * @param      mBit                 Mark bit.
 */
static void MakeRTPPacket(
        HAPRTPStream* stream,
        const uint8_t* payloadBytes,
        size_t numBytes,
        uint8_t* packetBytes,
        size_t maxBytes,
        size_t* numPacketBytes,
        uint16_t fragmentHeader,
        uint8_t cvoId,
        int mBit) {
    HAPPrecondition(stream);
    HAPPrecondition(payloadBytes);
    HAPPrecondition(packetBytes);
    HAPPrecondition(maxBytes >= 22);
    HAPPrecondition(numPacketBytes);

    uint8_t* payload;
    uint32_t index = stream->sender.baseIndex + stream->sender.totalPackets;
    size_t header = 0; // fragment header size

    // Fill header.
    packetBytes[0] = kRTPVersion << 6U; // P = 0, X = 0, CC = 0
    // NOLINTNEXTLINE(google-readability-casting)
    packetBytes[1] = (uint8_t)(stream->config.payloadType + (uint32_t)((uint32_t) mBit << 7U));
    packetBytes[2] = (uint8_t)(index >> 8U); // 16 bit sequence number
    packetBytes[3] = (uint8_t) index;
    HAPWriteBigUInt32(&packetBytes[4], stream->sender.timestamp);
    HAPWriteBigUInt32(&packetBytes[8], stream->config.localSSRC);
    size_t size = kRTPHeaderSize;

    // Check CVO header extension.
    if (cvoId) {
        packetBytes[0] = kRTPVersion << 6U | 1U << 4U; // P = 0, X = 1, CC = 0
        // Add one byte header extension.
        HAPWriteBigUInt32(&packetBytes[12],
                          (uint32_t)((uint32_t) kOneByteHeaderExt << 16U | 1U)); // size = 2 words
        HAPWriteBigUInt32(
                &packetBytes[16], (uint32_t)((uint32_t) cvoId << 28U | (uint32_t) stream->sender.cvoInfo << 16U));
        stream->sender.prevCvoInfo = stream->sender.cvoInfo;
        size += kCVOHeaderSize;
    }
    payload = packetBytes + size;

    // Insert payload header.
    if (fragmentHeader) {
        payload[0] = (uint8_t)(fragmentHeader >> 8U);
        payload[1] = (uint8_t) fragmentHeader;
        header = 2;
        size += 2;
    } else {
        payload[0] = payloadBytes[0]; // NAL header
    }

    // Packet size.
    size += numBytes;
    HAPAssert(size + stream->srtp.outputContext.tagSize <= maxBytes);

    // Log packet before encryption.
    HAPLogBufferDebug(&kHAPRTPController_PacketLog, packetBytes, size, "(%p) >", (const void*) stream);

    // Encryption.
    if (stream->srtp.outputContext.keySize) {
        HAPSRTPEncrypt(&stream->srtp.outputContext, payload, payloadBytes, header, numBytes, index);
        payload += header + numBytes;
        HAPSRTPAuthenticate(
                &stream->srtp.outputContext,
                payload, // append tag
                packetBytes,
                (size_t)(payload - packetBytes),
                index >> 16U); // ROC
        size += stream->srtp.outputContext.tagSize;
    } else {
        HAPRawBufferCopyBytes(payload + header, payloadBytes, numBytes);
    }

    // Statistics.
    stream->sender.totalPackets++;
    stream->sender.totalBytes += (uint32_t)(header + numBytes);

    *numPacketBytes = size;
}

/**
 * Poll H264 packet from RTP stream.
 *
 * @param      stream               HAP RTP stream.
 * @param      payloadBytes         Payload bytes.
 * @param      numPayloadBytes      Payload length.
 * @param[out] bytes                Buffer that will be filled with H264 data.
 * @param      maxBytes             Maximum number of bytes that may be filled into the buffer.
 * @param[out] numBytes             Effective number of bytes written to the buffer.
 */
static void PollH264Packet(
        HAPRTPStream* stream,
        const uint8_t* payloadBytes,
        size_t numPayloadBytes,
        uint8_t* bytes,
        size_t maxBytes, // MIN(buffer size, MTU)
        size_t* numBytes) {
    HAPPrecondition(stream);
    HAPPrecondition(payloadBytes);
    HAPPrecondition(bytes);

    size_t length = numPayloadBytes;
    int32_t offset = stream->sender.fragmentationOffset;
    uint8_t type = (uint8_t)(payloadBytes[0] & 0x1FU); // NAL header byte
    uint8_t nri = (uint8_t)(payloadBytes[0] & 0x60U);
    size_t maxPayload = maxBytes - stream->config.packetOverhead; // max payload size
    size_t maxLast = maxPayload;                                  // max payload size of last packet
    uint8_t cvoId = 0;

    if (offset == -1) {
        // Send sps/pps using STAP-A aggregation.
        uint8_t* data = bytes + kRTPHeaderSize;
        data[0] = (uint8_t)(nri + kH264TypeStapA);
        data[1] = (uint8_t)(stream->buffer.numSpsBytes >> 8U);
        data[2] = (uint8_t) stream->buffer.numSpsBytes;
        HAPRawBufferCopyBytes(&data[3], stream->buffer.spsBytes, stream->buffer.numSpsBytes);
        data += stream->buffer.numSpsBytes + 3;
        data[0] = (uint8_t)(stream->buffer.numPpsBytes >> 8U);
        data[1] = (uint8_t) stream->buffer.numPpsBytes;
        HAPRawBufferCopyBytes(&data[2], stream->buffer.ppsBytes, stream->buffer.numPpsBytes);
        length = stream->buffer.numSpsBytes + stream->buffer.numPpsBytes + 5;
        stream->sender.fragmentationOffset = 0; // continue with key frame payload
        MakeRTPPacket(stream, bytes + kRTPHeaderSize, length, bytes, maxBytes, numBytes, 0, 0, 0);
        return;
    }

    // Check CVO header extension.
    if (stream->sender.cvoId &&
        (type == kH264TypeIDR || (type < kH264TypeSPS && stream->sender.cvoInfo != stream->sender.prevCvoInfo))) {
        // reserve space for CVO header
        cvoId = stream->sender.cvoId;
        maxLast -= kCVOHeaderSize;
    }

    if (length <= maxLast) {
        // Use single NALU packet.
        int mBit = type < kH264TypeSPS;     // mark bit
        stream->sender.payloadBytes = NULL; // no more packets
        MakeRTPPacket(stream, payloadBytes, length, bytes, maxBytes, numBytes, 0, cvoId, mBit);
    } else {
        // Use FU-A fragmentation.
        uint16_t fragmentHeader = (uint16_t)((uint16_t)(nri + kH264TypeFuA) << 8U) | (uint16_t) type;
        int mBit = 0;
        maxPayload -= 2; // compensate fragmentation header size
        maxLast -= 2;
        length -= (uint32_t) offset;
        if (length > maxLast) {
            if (offset == 0) {
                // Send first fragment.
                offset = 1; // skip header byte
                length--;
                fragmentHeader |= 0x80U; // set start bit
            }
            length = (length <= maxPayload) ? (uint32_t) maxLast : (uint32_t) maxPayload;
            cvoId = 0;
        } else {
            // Send last fragment.
            fragmentHeader |= 0x40U;            // set stop bit
            mBit = type < kH264TypeSPS;         // mark bit
            stream->sender.payloadBytes = NULL; // no more packets
        }
        stream->sender.fragmentationOffset = offset + (int32_t) length;
        MakeRTPPacket(stream, payloadBytes + offset, length, bytes, maxBytes, numBytes, fragmentHeader, cvoId, mBit);
    }
}

void HAPRTPStreamPollPacket(
        HAPRTPStream* stream,
        void* bytes,
        size_t maxBytes, // buffer size
        size_t* numBytes) {
    HAPPrecondition(stream);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    size_t numPayloadBytes = stream->sender.numPayloadBytes;
    const uint8_t* payloadBytes = stream->sender.payloadBytes;
    if (payloadBytes == NULL) {
        // No data to send.
        *numBytes = 0;
        return;
    }

    // cap buffer size to max payload MTU
    if (maxBytes > stream->config.maxPacketMTU) {
        maxBytes = stream->config.maxPacketMTU;
    }

    if (stream->config.encodeType == kHAPRTPEncodeType_H264) {
        // Use h264 payload.
        PollH264Packet(stream, payloadBytes, numPayloadBytes, bytes, maxBytes, numBytes);

    } else if (stream->config.encodeType == kHAPRTPEncodeType_Simple) {
        // Use standard payload.
        stream->sender.payloadBytes = NULL; // no more data
        MakeRTPPacket(stream, payloadBytes, numPayloadBytes, bytes, maxBytes, numBytes, 0, 0, 0);

    } else {
        HAPLogError(&logObject, "Invalid stream encode type");
        HAPFatalError();
    }
}

/**
 * Handle RTCP Sender Report.
 *
 * @param      stream               HAP RTP stream.
 * @param      packet               RTP packet.
 * @param      actualTime           The actual time of the RTP packet ([ns] since 1970).
 */
static void HandleSenderReport(HAPRTPStream* stream, const uint8_t* packet, HAPShortNTP actualTime) {
    HAPNTPTime ntpTimestamp = HAPReadBigUInt64(packet);
    uint32_t timestamp = HAPReadBigUInt32(&packet[8]);

    // uint32_t packetCount = GetUInt32(&packet[12]);
    // uint32_t octetCount = GetUInt32(&packet[16]);

    // Statistics.
    stream->receiver.lastSRTime = actualTime; // short NTP time
    stream->receiver.lastSRNTPStamp = ntpTimestamp;
    stream->receiver.lastSRTimestamp = timestamp;
}

/**
 * Handle RTCP Receiver Report.
 *
 * @param      stream               HAP RTP stream.
 * @param      packet               RTP packet.
 * @param      actualTime           The actual time of the RTP packet ([ns] since 1970).
 */
static void HandleReceiverReport(HAPRTPStream* stream, const uint8_t* packet, HAPShortNTP actualTime) {
    uint32_t ssrc = HAPReadBigUInt32(packet);
    if (ssrc == stream->config.localSSRC) {
        // uint8_t fractionLost = packet[4];
        // uint32_t packetsLost = (int32_t) GetUInt32(&packet[4]) << 8U >> 8U; // signed 24 bit
        // uint32_t maxNum = GetUInt32(&packet[8]);
        // uint32_t jitter = GetUInt32(&packet[12]);
        HAPShortNTP lastSRTime = HAPReadBigUInt32(&packet[16]);  // short NTP time (% 2^16s)
        HAPShortNTP lastSRDelay = HAPReadBigUInt32(&packet[20]); // short NTP time (% 2^16s)

        // Statistics.
        if (lastSRTime && lastSRDelay) {
            // Get roundtrip time.
            HAPShortNTP rtt = actualTime - lastSRTime - lastSRDelay;
            // Store decaying maximum.
            stream->receiver.roundTripTime -= stream->receiver.roundTripTime >> 4U;
            if (rtt > stream->receiver.roundTripTime) {
                stream->receiver.roundTripTime = rtt;
            }
        }
    }
}

/**
 * Handle RTCP TMMBR (Temporary Maximum Media Stream Bit Rate Request).
 *
 * @param      stream               HAP RTP stream.
 * @param      bytes                RTP packet.
 */
static void HandleTMMBR(HAPRTPStream* stream, const uint8_t* bytes) {
    uint32_t entry, mantissa, exponent, overhead, rateOverhead, bitrate, payload;

    if (HAPReadBigUInt32(&bytes[0]) != stream->config.localSSRC) {
        return; // wrong ssrc
    }

    entry = HAPReadBigUInt32(&bytes[4]);
    exponent = entry >> 26U;
    mantissa = (entry >> 9U) & 0x1FFFFU;
    overhead = entry & 0x1FFU; // packet overhead
    bitrate = mantissa << exponent;
    // Estimate bitrate overhead.
    payload = stream->config.maxPacketMTU - stream->config.packetOverhead;
    rateOverhead = bitrate * overhead / payload;
    // get encoder bitrate
    bitrate -= rateOverhead;

    // Handle overflow.
    if (mantissa > 0xFFFFFFFF >> exponent || bitrate > kMaxBitRate) {
        // use maximum supported rate
        bitrate = kMaxBitRate;
        rateOverhead = bitrate * overhead / payload;
        mantissa = bitrate + rateOverhead;
        exponent = 0;
        while (mantissa > 0x1FFFF) {
            mantissa >>= 1U;
            exponent++;
        }
        entry = exponent << 26U | mantissa << 9U | overhead;
    }

    stream->control.tmmbnEntry = entry;   // trigger TMMBN feedback
    stream->control.newBitRate = bitrate; // new encoder bitrate
    if (bitrate != stream->control.actualBitRate) {
        HAPLogInfo(
                &logObject,
                "new bitrate requested: %lu -> %lu",
                (unsigned long) stream->control.actualBitRate,
                (unsigned long) bitrate);
    }
}

/**
 * Handle RTCP PLI (Picture Loss Indication).
 *
 * @param      stream               HAP RTP stream.
 * @param      bytes                RTP packet.
 * @param      actualTime           The actual time of the RTP packet ([ns] since 1970).
 */
static void HandlePLI(
        HAPRTPStream* stream,
        const uint8_t* bytes,
        HAPEpochTime actualTime) // [ns] since 1970
{
    char* str;
    if (HAPReadBigUInt32(&bytes[0]) != stream->config.localSSRC) {
        return; // wrong ssrc
    }

    if (stream->sender.keyFrame != kKeyFrameIdle) {
        HAPLogInfo(&logObject, "PLI: key frame already requested");
    } else {
        if (actualTime >=
            stream->sender.keyFrameTime + HAPNTPTimeToNS((uint64_t) stream->receiver.roundTripTime << 17U)) {
            // not within 2 RTT of last key frame
            stream->sender.keyFrame = kKeyFrameRequest; // request new key frame
            str = "request new key frame";
        } else {
            str = "no new key frame";
        }

        HAPNTPTime act = HAPEpochTimeToHAPNTPTime(actualTime);
        HAPNTPTime key = HAPEpochTimeToHAPNTPTime(stream->sender.keyFrameTime);
        HAPLogInfo(
                &logObject,
                "PLI: %s, time=%lu.%03lu, key-time=%lu.%03lu, rtt=%lu.%03lu",
                str,
                (unsigned long) (act >> 32U) & 0xFFFFU,
                (unsigned long) ((uint32_t) act * 1000ULL >> 32U),
                (unsigned long) (key >> 32U) & 0xFFFFU,
                (unsigned long) ((uint32_t) key * 1000ULL >> 32U),
                (unsigned long) stream->receiver.roundTripTime >> 16U,
                (unsigned long) (stream->receiver.roundTripTime & 0xFFFFU) * 1000U >> 16U);
    }
}

/**
 * Handle RTCP FIR (Full Intra Request).
 *
 * @param      stream               HAP RTP stream.
 * @param      bytes                RTP packet.
 */
static void HandleFIR(HAPRTPStream* stream, const uint8_t* bytes) {
    uint32_t entry;

    if (HAPReadBigUInt32(&bytes[0]) != stream->config.localSSRC) {
        return; // wrong ssrc
    }
    entry = bytes[4];
    if (entry == stream->control.lastFIRSeqNum) {
        return; // repeated FIR packet
    }

    stream->control.lastFIRSeqNum = entry;
    if (stream->sender.keyFrame != kKeyFrameIdle) {
        HAPLogInfo(&logObject, "FIR: key frame already requested");
    } else {
        stream->sender.keyFrame = kKeyFrameRequest; // request new key frame
        HAPLogInfo(&logObject, "FIR: request new key frame");
    }
}

/**
 * Handle RTCP TSTR (Temporal-Spatial Trade-off Request).
 *
 * @param      stream               HAP RTP stream.
 * @param      bytes                RTP packet.
 */
static void HandleTSTR(HAPRTPStream* stream, const uint8_t* bytes) {
    uint32_t entry;

    if (HAPReadBigUInt32(&bytes[0]) != stream->config.localSSRC) {
        return; // wrong ssrc
    }
    entry = bytes[4];
    if (entry == stream->control.lastTSTRSeqNum) {
        return; // repeated TSTR packet
    }

    stream->control.lastTSTRSeqNum = entry;
    // Trigger TSTN feedback with highest sequence number.
    if (stream->control.tstnEntry == kInvalid || (int8_t)(entry - (stream->control.tstnEntry >> 24U)) > 0) {
        stream->control.tstnEntry = (entry << 24U) + 31; // we always work at full frame rate
    }
    HAPLogInfo(&logObject, "TSTR received: index=%d", bytes[7] & 0x1FU);
}

/**
 * Handle the RTCP packet.
 *
 * @param      stream               HAP RTP stream.
 * @param      bytes                RTCP packet.
 * @param      numBytes             Length of the RTCP packet.
 * @param      actualTime           The actual time ([ns] since 1970).
 * @param      ntpTime              The ntp time.
 */
static void HandleRTCPPacket(
        HAPRTPStream* stream,
        const uint8_t* bytes,
        size_t numBytes,
        HAPEpochTime actualTime, // [ns] since 1970
        HAPShortNTP ntpTime) {
    uint8_t type = bytes[1];
    uint8_t rc = (uint8_t)(bytes[0] & 0x1FU); // receiver report count

    switch (type) {
        case kPacketSR:   // SR packet
        case kPacketRR: { // RR packet
            if (numBytes < 8) {
                return; // wrong header size
            }
            if (HAPReadBigUInt32(&bytes[4]) != stream->config.remoteSSRC) {
                return; // wrong source
            }
            bytes += 8; // first report
            if (type == kPacketSR) {
                if (numBytes != rc * 24U + 28U) {
                    return; // wrong size
                }
                HandleSenderReport(stream, bytes, ntpTime);
                bytes += 20;
            } else {
                if (numBytes != rc * 24U + 8U) {
                    return; // wrong size
                }
            }
            while (rc > 0) {
                HandleReceiverReport(stream, bytes, ntpTime);
                bytes += 24;
                rc--;
            }
            break;
        }
        case kPacketRTPFB:  // RTPFB payload
        case kPacketPSFB: { // PSFB payload
            if (numBytes < 12) {
                return; // wrong header size
            }
            if (HAPReadBigUInt32(&bytes[4]) != stream->config.remoteSSRC) {
                return; // wrong source
            }
            if (type == kPacketRTPFB && rc == kRTPFB_TMMBR) { // TMMBR packet
                if (numBytes != 20) {
                    return; // wrong size
                }
                HandleTMMBR(stream, &bytes[12]);
            } else if (type == kPacketPSFB && rc == kPSFB_PLI) { // PLI packet
                if (numBytes != 12) {
                    return; // wrong size
                }
                HandlePLI(stream, &bytes[8], actualTime);
            } else if (type == kPacketPSFB && rc == kPSFB_FIR) { // FIR packet
                if (numBytes != 20) {
                    return; // wrong size
                }
                HandleFIR(stream, &bytes[12]);
            } else if (type == kPacketPSFB && rc == kPSFB_TSTR) { // TSTR packet
                if (numBytes != 20) {
                    return; // wrong size
                }
                HandleTSTR(stream, &bytes[12]);
            } else {
                // Ignore packet.
            }
            break;
        }
        default: {
            // Ignore other packets (including SDES).
            break;
        }
    }
}

/**
 * Check whether a RTCP packet is valid.
 *
 * @param      stream               Stream against which the packet is tested.
 * @param      bytes                Packet.
 * @param      numBytes             Packet size.
 *
 * @return                          Whether the RTCP packet is valid.
 */
static bool ValidRTCPPacket(const HAPRTPStream* stream, const uint8_t* bytes, size_t numBytes) {
    uint32_t minLength;

    // Check RTCP header.
    if (stream->srtcp.inputContext.keySize) {
        minLength = 12 + stream->srtp.inputContext.tagSize;
    } else {
        minLength = 8;
    }

    if (numBytes < minLength) {
        return false; // wrong length
    }
    if ((bytes[0] & 0x20U) != 0) {
        return false; // wrong P bit
    }
    if ((bytes[1] & 0xFEU) != kPacketSR) {
        return false; // wrong packet type
    }

    return true;
}

/**
 * Push a RTCP packet into the stream.
 *
 * @param      stream               The HAP RTP stream where the data is pushed to.
 * @param      bytes                RAW buffer where the RTCP stream is read. The buffer
 *                                  is stored in the HAP RTCP stream.
 * @param      numBytes             Length of the buffer.
 * @param      actualTime           The actual time ([ns] since 1970).
 */
static void PushRTCPPacket(
        HAPRTPStream* stream,
        uint8_t* bytes,
        size_t numBytes,
        HAPEpochTime actualTime) // [ns] since 1970
{
    HAPPrecondition(numBytes > stream->srtcp.inputContext.tagSize);

    HAPNTPTime ntpTime = HAPEpochTimeToHAPNTPTime(actualTime);
    HAPShortNTP shortNtp = (uint32_t)(ntpTime >> 16U);
    size_t length = numBytes;

    // Check SSRC before decoding.
    if (HAPReadBigUInt32(&bytes[4]) != stream->config.remoteSSRC) {
        return;
    }

    // Decryption.
    if (stream->srtcp.inputContext.keySize) {
        size_t tag = length - stream->srtcp.inputContext.tagSize;
        length = tag - 4;
        uint32_t index = HAPReadBigUInt32(bytes + length);
        if (!HAPSRTPVerifyAuthentication(&stream->srtcp.inputContext, bytes + tag, bytes, length, index)) {
            HAPLogError(&logObject, "SRTCP input authentication failed");
            return;
        }
        uint32_t eBit = index >> 31U;
        index &= 0x7FFFFFFFU;

        // Replay protection.
        int32_t delta = (int32_t)(index - stream->receiver.maxRtcpIndex);
        if (delta > 0) {
            // Update map.
            stream->receiver.replayMap = (delta < 32) ? (stream->receiver.replayMap << (uint32_t) delta | 1U) : 1;
            stream->receiver.maxRtcpIndex = index;
        } else if (delta > -32) {
            // Check for replay.
            uint32_t bit = (uint32_t) 1U << (uint32_t) -delta;
            if (stream->receiver.replayMap & bit) {
                HAPLogError(&logObject, "Replayed SRTCP input packet");
                return;
            }
            // Update map.
            stream->receiver.replayMap |= bit;
        } else { // delta <= -32
            // Ignore outdated packets.
            return;
        }

        if (eBit) { // packet encrypted
            HAPSRTPDecrypt(
                    &stream->srtcp.inputContext,
                    bytes + kRTCPHeaderSize,
                    bytes + kRTCPHeaderSize,
                    length - kRTCPHeaderSize,
                    index);
        }
    }

    // Log packet after decryption.
    HAPLogBufferDebug(&kHAPRTPController_PacketLog, bytes, length, "(%p) <", (const void*) stream);

    // Check RTCP packet list consistency.
    const uint8_t* end = bytes + length;
    const uint8_t* packet = bytes;
    while (packet + 4 <= end) {
        if (packet[0] >> 6U != kRTPVersion) {
            return; // wrong version
        }
        uint32_t size = (uint32_t)((packet[2] << 10U) + (packet[3] << 2U) + 4);
        packet += size;
    }
    if (packet != end) {
        return; // wrong length
    }

    stream->receiver.lastPacketTime = (uint32_t)(ntpTime >> 32U); // [s]

    // Handle all RTCP packets.
    packet = bytes;
    while (packet < end) {
        uint32_t size = (uint32_t)((packet[2] << 10U) + (packet[3] << 2U) + 4);
        HandleRTCPPacket(stream, packet, size, actualTime, shortNtp);
        packet += size;
    }
}

/**
 * Check whether a RTP packet is valid.
 *
 * @param      stream               Stream against which the packet is tested.
 * @param      bytes                Packet buffer.
 * @param      numBytes             Packet buffer size.
 *
 * @return                          Whether the RTP packet is valid.
 */
HAP_RESULT_USE_CHECK
static bool ValidRTPPacket(const HAPRTPStream* stream, const uint8_t* bytes, size_t numBytes) {
    // Standard checks.
    uint32_t minLength = kRTPHeaderSize + stream->srtp.inputContext.tagSize;
    if (numBytes < minLength) {
        return false; // wrong length
    }
    if (bytes[0] >> 6U != kRTPVersion) {
        return false; // wrong version
    }
    if ((bytes[0] & 0x1FU) != 0) {
        return false; // wrong X/CC
    }

    // Specific checks.
    uint32_t payloadType = bytes[1] & 0x7FU;
    if (payloadType != stream->config.payloadType) {
        return false; // wrong payload type
    }
    uint32_t ssrc = HAPReadBigUInt32(&bytes[8]);
    if (ssrc != stream->config.remoteSSRC) {
        return false; // wrong SSRC
    }

    return true;
}

/**
 * Push a RTP packet to a HAP RTP stream.
 *
 * @param      stream               The HAP RTP stream where the data is pushed to.
 * @param      bytes                RAW buffer where the RTP packet is read. The buffer
 *                                  is stored in the HAP RTP packet.
 * @param      numBytes             Length of the packet.
 * @param[out] numPayloadBytes      Number of payload bytes in the packet.
 * @param      actualTime           The actual time ([ns] since 1970).
 */
static void PushRTPPacket(
        HAPRTPStream* stream,
        uint8_t* bytes,
        size_t numBytes,
        size_t* numPayloadBytes,
        HAPEpochTime actualTime) {
    HAPTimestamp timestamp; // data timestamp
    HAPTimestamp receiveTime;
    HAPTimestamp transitTime;
    uint32_t sequence;
    int32_t delta;

    // We do not expect video input.
    if (stream->config.encodeType == kHAPRTPEncodeType_H264) {
        *numPayloadBytes = 0;
        return;
    }

    timestamp = HAPReadBigUInt32(&bytes[4]);
    sequence = (uint32_t)((bytes[2] << 8U) + bytes[3]); // 16 bit sequence number
    if (stream->receiver.totalPackets == 0) {           // first data
        delta = 0;
        stream->receiver.firstIndex = sequence; // initialize index calculation
        stream->receiver.lastIndex = sequence;
        stream->receiver.referenceTimestamp = timestamp; // initialize timestamp reference
        stream->receiver.referenceHAPTimeNS = 0;
    } else {
        delta = (int16_t)(sequence - stream->receiver.lastIndex); // sequence number change
        if (delta <= 0) {
            *numPayloadBytes = 0;
            return; // ignore duplicate and late packets
        }
        stream->receiver.lastIndex += (uint32_t) delta; // new index (ROC||sequence)
    }

    // Check authentication.
    if (stream->srtp.inputContext.keySize) {
        numBytes -= stream->srtp.inputContext.tagSize;
        if (!HAPSRTPVerifyAuthentication(
                    &stream->srtp.inputContext,
                    bytes + numBytes, // tag
                    bytes,
                    numBytes,
                    stream->receiver.lastIndex >> 16U)) // include ROC
        {
            *numPayloadBytes = 0;
            HAPLogError(&logObject, "SRTP input authentication failed");
            return;
        }
    }

    stream->receiver.lastPacketTime = (uint32_t)(HAPEpochTimeToHAPNTPTime(actualTime) >> 32U); // [s];

    // Log packet.
    HAPLogBufferDebug(&kHAPRTPController_PacketLog, bytes, numBytes, "(%p) <", (const void*) stream);

    // jitter and reorder buffer ???

    if (delta > 5) {
        // Dropout -> reset output stream time.
        stream->receiver.referenceTimestamp = timestamp;
        stream->receiver.referenceHAPTimeNS = 0;
    }

    // Skip header.
    bytes += kRTPHeaderSize;
    numBytes -= kRTPHeaderSize;

    // Store payload reference for decoding.
    stream->receiver.payloadBytes = bytes;
    stream->receiver.numPayloadBytes = (uint32_t)(numBytes);
    stream->receiver.timestamp = timestamp;

    // Statistics.
    if (stream->receiver.lastIndex > stream->receiver.highIndex) {
        stream->receiver.highIndex = stream->receiver.lastIndex;
    }
    stream->receiver.totalPackets++;
    receiveTime = TimeToTimestamp(stream, actualTime); // [timestamp units]
    transitTime = receiveTime - timestamp;             // [timestamp units]
    delta = (int32_t)(transitTime - stream->receiver.lastTransitTime);
    stream->receiver.lastTransitTime = transitTime;
    if (delta < 0) {
        delta = -delta;
    }
    // Jitter calculation according to RFC3550 A.8.
    stream->receiver.scaledJitter += (uint32_t) delta - ((stream->receiver.scaledJitter + 8) >> 4U);

    *numPayloadBytes = numBytes;
}

void HAPRTPStreamPushPacket(
        HAPRTPStream* stream,
        void* bytes, // may be overwritten
        size_t numBytes,
        size_t* numPayloadBytes,
        HAPEpochTime actualTime) // [ns] since 1970
{
    HAPPrecondition(stream);
    HAPPrecondition(bytes);

    if (ValidRTCPPacket(stream, bytes, numBytes)) {
        // RTCP packets are control packets and thus do not contain RTP data.
        *numPayloadBytes = 0;
        PushRTCPPacket(stream, bytes, numBytes, actualTime);
    } else if (ValidRTPPacket(stream, bytes, numBytes)) {
        PushRTPPacket(stream, bytes, numBytes, numPayloadBytes, actualTime);
    } else {
        // Do nothing.
    }
}

void HAPRTPStreamPollPayload(
        HAPRTPStream* stream,
        void* bytes,
        size_t maxBytes, // buffer size
        size_t* numBytes,
        HAPTimeNS* sampleTime) // [ns] from stream start
{
    HAPPrecondition(stream);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);
    HAPPrecondition(sampleTime);

    uint8_t* payloadBytes = stream->receiver.payloadBytes;
    if (payloadBytes == NULL) {
        // No data to send.
        *numBytes = 0;
        return;
    }

    size_t numPayloadBytes = stream->receiver.numPayloadBytes;
    HAPAssert(numPayloadBytes <= maxBytes);

    stream->receiver.payloadBytes = NULL;
    stream->receiver.numPayloadBytes = 0;

    // Decryption.
    if (stream->srtp.inputContext.keySize) {
        HAPSRTPDecrypt(&stream->srtp.inputContext, bytes, payloadBytes, numPayloadBytes, stream->receiver.lastIndex);
    } else {
        HAPRawBufferCopyBytes(bytes, payloadBytes, numPayloadBytes);
    }

    *numBytes = numPayloadBytes;
    *sampleTime = TimestampToNS(stream, stream->receiver.timestamp);
}
