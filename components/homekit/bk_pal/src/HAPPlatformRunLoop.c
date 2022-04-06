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

#include <errno.h>
#include <string.h>

#include "bk_api_mem.h"
#include "lwip/sockets.h"

#include "HAPPlatform+Init.h"
#include "HAPPlatform.h"
#include "HAPPlatformFileHandle.h"
#include "HAPPlatformLog+Init.h"
#include "HAPPlatformRunLoop+Init.h"

#define LOOPBACK_PORT   12321

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "RunLoop" };

/**
 * Internal file handle type, representing the registration of a platform-specific file descriptor.
 */
typedef struct HAPPlatformFileHandle HAPPlatformFileHandle;

/**
 * Internal file handle representation.
 */
struct HAPPlatformFileHandle {
	/**
	 * Platform-specific file descriptor.
	 */
	int fileDescriptor;

	/**
	 * Set of file handle events on which the callback shall be invoked.
	 */
	HAPPlatformFileHandleEvent interests;

	/**
	 * Function to call when one or more events occur on the given file descriptor.
	 */
	HAPPlatformFileHandleCallback callback;

	/**
	 * The context parameter given to the HAPPlatformFileHandleRegister function.
	 */
	void* _Nullable context;

	/**
	 * Previous file handle in linked list.
	 */
	HAPPlatformFileHandle* _Nullable prevFileHandle;

	/**
	 * Next file handle in linked list.
	 */
	HAPPlatformFileHandle* _Nullable nextFileHandle;

	/**
	 * Flag indicating whether the platform-specific file descriptor is registered with an I/O multiplexer or not.
	 */
	bool isAwaitingEvents;
};

/**
 * Internal timer type.
 */
typedef struct HAPPlatformTimer HAPPlatformTimer;

/**
 * Internal timer representation.
 */
struct HAPPlatformTimer {
	/** Deadline at which the timer expires. */
	HAPTime deadline;

	/** Callback that is invoked when the timer expires. */
	HAPPlatformTimerCallback callback;

	/** The context parameter given to the HAPPlatformTimerRegister function. */
	void* _Nullable context;

	/** Next timer in linked list. */
	HAPPlatformTimer* _Nullable nextTimer;
};

/**
 * Run loop state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformRunLoopState) {
							/** Idle. */
							kHAPPlatformRunLoopState_Idle,

							/** Running. */
							kHAPPlatformRunLoopState_Running,

							/** Stopping. */
							kHAPPlatformRunLoopState_Stopping
} HAP_ENUM_END(uint8_t, HAPPlatformRunLoopState);

static struct {
	/** Sentinel node of a circular doubly-linked list of file handles. */
	HAPPlatformFileHandle fileHandleSentinel;

	/** Pointer to sentinel node, representing a circular doubly-linked list of file handles. */
	HAPPlatformFileHandle* _Nullable fileHandles;

	/** File handle cursor, used to handle reentrant modifications of global file handle list during iteration. */
	HAPPlatformFileHandle* _Nullable fileHandleCursor;

	/** Start of linked list of timers, ordered by deadline. */
	HAPPlatformTimer* _Nullable timers;

	/**
	 * Loopback file descriptor to receive data.
	 */
	volatile int loopbackFileDescriptor;

	/**
	 * Self-pipe byte buffer.
	 *
	 * - Callbacks are serialized into the buffer as:
	 *   - 8-byte aligned callback pointer.
	 *   - Context size (up to UINT8_MAX).
	 *   - Context (unaligned). When invoking the callback, the context is first moved to be 8-byte aligned.
	 */
	HAP_ALIGNAS(8)
	char loopbackBytes[sizeof(HAPPlatformRunLoopCallback) + 1 + UINT8_MAX];

	/** Number of bytes in self-pipe byte buffer. */
	size_t numLoopbackBytes;

	/** File handle for self-pipe. */
	HAPPlatformFileHandleRef loopbackFileHandle;

	/** Current run loop state. */
	HAPPlatformRunLoopState state;
} runLoop = {.fileHandleSentinel = {.fileDescriptor = -1,
													.interests = {.isReadyForReading = false,
																.isReadyForWriting = false,
																.hasErrorConditionPending = false},
													.callback = NULL,
													.context = NULL,
													.prevFileHandle = &runLoop.fileHandleSentinel,
													.nextFileHandle = &runLoop.fileHandleSentinel,
													.isAwaitingEvents = false},
								.fileHandles = &runLoop.fileHandleSentinel,
								.fileHandleCursor = &runLoop.fileHandleSentinel,
								.timers = NULL,
								.loopbackFileDescriptor = -1 };

HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileHandleRegister(
		HAPPlatformFileHandleRef* fileHandle,
		int fileDescriptor,
		HAPPlatformFileHandleEvent interests,
		HAPPlatformFileHandleCallback callback,
		void* _Nullable context)
{
	HAPPrecondition(fileHandle);

	// Prepare fileHandle.
	HAPPlatformFileHandle* fileHandle_p = os_malloc(sizeof(HAPPlatformFileHandle));
	if (!fileHandle_p) {
		HAPLog(&logObject, "Cannot allocate more file handles.");
		*fileHandle = 0;
		return kHAPError_OutOfResources;
	}
	fileHandle_p->fileDescriptor = fileDescriptor;
	fileHandle_p->interests = interests;
	fileHandle_p->callback = callback;
	fileHandle_p->context = context;
	fileHandle_p->prevFileHandle = runLoop.fileHandles->prevFileHandle;
	fileHandle_p->nextFileHandle = runLoop.fileHandles;
	fileHandle_p->isAwaitingEvents = false;
	HAPAssert(runLoop.fileHandles);
	HAPAssert(runLoop.fileHandles->prevFileHandle);
	runLoop.fileHandles->prevFileHandle->nextFileHandle = fileHandle_p;
	runLoop.fileHandles->prevFileHandle = fileHandle_p;

	*fileHandle = (HAPPlatformFileHandleRef)fileHandle_p;
	return kHAPError_None;
}

void HAPPlatformFileHandleUpdateInterests(
		HAPPlatformFileHandleRef fileHandle,
		HAPPlatformFileHandleEvent interests,
		HAPPlatformFileHandleCallback callback,
		void* _Nullable context)
{
	HAPPrecondition(fileHandle);
	HAPPlatformFileHandle* fileHandle_p = (HAPPlatformFileHandle * _Nonnull)fileHandle;

	fileHandle_p->interests = interests;
	fileHandle_p->callback = callback;
	fileHandle_p->context = context;
}

void HAPPlatformFileHandleDeregister(HAPPlatformFileHandleRef fileHandle)
{
	HAPPrecondition(fileHandle);
	HAPPlatformFileHandle* fileHandle_p = (HAPPlatformFileHandle * _Nonnull)fileHandle;

	HAPPrecondition(fileHandle_p->prevFileHandle);
	HAPPrecondition(fileHandle_p->nextFileHandle);

	if (fileHandle_p == runLoop.fileHandleCursor) {
		runLoop.fileHandleCursor = fileHandle_p->nextFileHandle;
	}

	fileHandle_p->prevFileHandle->nextFileHandle = fileHandle_p->nextFileHandle;
	fileHandle_p->nextFileHandle->prevFileHandle = fileHandle_p->prevFileHandle;

	fileHandle_p->fileDescriptor = -1;
	fileHandle_p->interests.isReadyForReading = false;
	fileHandle_p->interests.isReadyForWriting = false;
	fileHandle_p->interests.hasErrorConditionPending = false;
	fileHandle_p->callback = NULL;
	fileHandle_p->context = NULL;
	fileHandle_p->nextFileHandle = NULL;
	fileHandle_p->prevFileHandle = NULL;
	fileHandle_p->isAwaitingEvents = false;
	os_free(fileHandle_p);
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformTimerRegister(
		HAPPlatformTimerRef* timer,
		HAPTime deadline,
		HAPPlatformTimerCallback callback,
		void* _Nullable context)
{
	HAPPrecondition(timer);
	HAPPlatformTimer* _Nullable* newTimer = (HAPPlatformTimer * _Nullable*)timer;
	HAPPrecondition(callback);

	// Prepare timer.
	*newTimer = os_malloc(sizeof(HAPPlatformTimer));
	if (!*newTimer) {
		HAPLog(&logObject, "Cannot allocate more timers.");
		return kHAPError_OutOfResources;
	}
	(*newTimer)->deadline = deadline ? deadline : 1;
	(*newTimer)->callback = callback;
	(*newTimer)->context = context;

	// Insert timer.
	for (HAPPlatformTimer* _Nullable* nextTimer = &runLoop.timers;; nextTimer = &(*nextTimer)->nextTimer) {
		if (!*nextTimer) {
			(*newTimer)->nextTimer = NULL;
			*nextTimer = *newTimer;
			break;
		}
		if ((*nextTimer)->deadline > deadline) {
			// Search condition must be '>' and not '>=' to ensure that timers fire in ascending order of their
			// deadlines and that timers registered with the same deadline fire in order of registration.
			(*newTimer)->nextTimer = *nextTimer;
			*nextTimer = *newTimer;
			break;
		}
	}

	return kHAPError_None;
}

void HAPPlatformTimerDeregister(HAPPlatformTimerRef timer)
{
	HAPPrecondition(timer);
	HAPPlatformTimer* timer_p = (HAPPlatformTimer*)timer;

	// Find and remove timer.
	for (HAPPlatformTimer* _Nullable* nextTimer = &runLoop.timers; *nextTimer; nextTimer = &(*nextTimer)->nextTimer) {
		if (*nextTimer == timer_p) {
			*nextTimer = timer_p->nextTimer;
			os_free(timer_p);
			return;
		}
	}

	// Timer not found.
	HAPFatalError();
}

static void CloseLoopback(int fileDescriptor)
{
	if (fileDescriptor != -1) {
		HAPLogDebug(&logObject, "close(%d);", fileDescriptor);
		int e = close(fileDescriptor);
		if (e != 0) {
			int _errno = errno;
			HAPAssert(e == -1);
			HAPPlatformLogPOSIXError(kHAPLogType_Error, "Closing loopback failed (log, fileDescriptor0).",
				_errno, __func__, HAP_FILE, __LINE__);
		}
	}
}

static void HandleLoopbackFileHandleCallback(
		HAPPlatformFileHandleRef fileHandle,
		HAPPlatformFileHandleEvent fileHandleEvents,
		void* _Nullable context HAP_UNUSED)
{
	HAPAssert(fileHandle);
	HAPAssert(fileHandle == runLoop.loopbackFileHandle);
	HAPAssert(fileHandleEvents.isReadyForReading);

	HAPAssert(runLoop.numLoopbackBytes < sizeof runLoop.loopbackBytes);

	size_t n;
	do {
		n = recvfrom(runLoop.loopbackFileDescriptor,
				&runLoop.loopbackBytes[runLoop.numLoopbackBytes],
				sizeof runLoop.loopbackBytes - runLoop.numLoopbackBytes, 0, NULL, NULL);
	} while (n == -1 && errno == EINTR);
	if (n == -1 && errno == EAGAIN) {
		return;
	}
	if (n < 0) {
		int _errno = errno;
		HAPAssert(n == -1);
		HAPPlatformLogPOSIXError(kHAPLogType_Error, "Self pipe read failed.", _errno, __func__, HAP_FILE, __LINE__);
		HAPFatalError();
	}
	if (n == 0) {
		HAPLogError(&logObject, "Self pipe read returned EOF.");
		HAPFatalError();
	}

	HAPAssert((size_t)n <= sizeof runLoop.loopbackBytes - runLoop.numLoopbackBytes);
	runLoop.numLoopbackBytes += (size_t)n;
	for (;;) {
		if (runLoop.numLoopbackBytes < sizeof(HAPPlatformRunLoopCallback) + 1) {
			break;
		}
		size_t contextSize = (size_t)runLoop.loopbackBytes[sizeof(HAPPlatformRunLoopCallback)];
		if (runLoop.numLoopbackBytes < sizeof(HAPPlatformRunLoopCallback) + 1 + contextSize) {
			break;
		}

		HAPPlatformRunLoopCallback callback;
		HAPRawBufferCopyBytes(&callback, &runLoop.loopbackBytes[0], sizeof(HAPPlatformRunLoopCallback));
		HAPRawBufferCopyBytes(&runLoop.loopbackBytes[0],
				&runLoop.loopbackBytes[sizeof(HAPPlatformRunLoopCallback) + 1],
				runLoop.numLoopbackBytes - (sizeof(HAPPlatformRunLoopCallback) + 1));
		runLoop.numLoopbackBytes -= (sizeof(HAPPlatformRunLoopCallback) + 1);

		// Issue memory barrier to ensure visibility of data referenced by callback context.
		//__atomic_signal_fence(__ATOMIC_SEQ_CST);
		//__atomic_thread_fence(__ATOMIC_SEQ_CST);

		callback(contextSize ? &runLoop.loopbackBytes[0] : NULL, contextSize);

		HAPRawBufferCopyBytes(&runLoop.loopbackBytes[0], &runLoop.loopbackBytes[contextSize],
					runLoop.numLoopbackBytes - contextSize);
		runLoop.numLoopbackBytes -= contextSize;
	}
}

void HAPPlatformRunLoopCreate(const HAPPlatformRunLoopOptions* options)
{
	HAPPrecondition(options);
	HAPPrecondition(options->keyValueStore);
	HAPError err;

	HAPLogDebug(&logObject, "Storage configuration: runLoop = %lu", (unsigned long) sizeof runLoop);
	HAPLogDebug(&logObject, "Storage configuration: fileHandle = %lu", (unsigned long) sizeof(HAPPlatformFileHandle));
	HAPLogDebug(&logObject, "Storage configuration: timer = %lu", (unsigned long) sizeof(HAPPlatformTimer));

	// Open loop back

	HAPPrecondition(runLoop.loopbackFileDescriptor == -1);
	int fileDescriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fileDescriptor < 0) {
		int _errno = errno;
		HAPPlatformLogPOSIXError(kHAPLogType_Error,
			"Loopback creation failed (log, call 'socket').",
			_errno, __func__, HAP_FILE, __LINE__);
		HAPFatalError();
	}
	int e = fcntl(fileDescriptor, F_SETFL, O_NONBLOCK);
	if (e == -1) {
		HAPPlatformLogPOSIXError(kHAPLogType_Error,
			"System call 'fcntl' to set loopback recv file descriptor flags to 'non-blocking' failed.",
			errno, __func__, HAP_FILE, __LINE__);
		HAPFatalError();
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(LOOPBACK_PORT);
	inet_aton("127.0.0.1", &addr.sin_addr);
	if (bind(fileDescriptor, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		CloseLoopback(fileDescriptor);
		int _errno = errno;
		HAPPlatformLogPOSIXError(kHAPLogType_Error,
			"Loopback socket bind failed (log, call 'bind').",
			_errno, __func__, HAP_FILE, __LINE__);
		HAPFatalError();
	}

	runLoop.loopbackFileDescriptor = fileDescriptor;

	err = HAPPlatformFileHandleRegister(&runLoop.loopbackFileHandle,
		runLoop.loopbackFileDescriptor,
		(HAPPlatformFileHandleEvent) {
			.isReadyForReading = true,
			.isReadyForWriting = false,
			.hasErrorConditionPending = false
		},
		HandleLoopbackFileHandleCallback, NULL);
	if (err) {
		HAPAssert(err == kHAPError_OutOfResources);
		HAPLogError(&logObject, "Failed to register loopback file handle.");
		HAPFatalError();
	}
	HAPAssert(runLoop.loopbackFileHandle);

	runLoop.state = kHAPPlatformRunLoopState_Idle;
	
	// Issue memory barrier to ensure visibility of write to runLoop.selfPipeFileDescriptor1 on other threads.
	//__sync_synchronize();
}

void HAPPlatformRunLoopRelease(void)
{
	CloseLoopback(runLoop.loopbackFileDescriptor);

	runLoop.loopbackFileDescriptor = -1;

	if (runLoop.loopbackFileHandle) {
		HAPPlatformFileHandleDeregister(runLoop.loopbackFileHandle);
		runLoop.loopbackFileHandle = 0;
	}

	runLoop.state = kHAPPlatformRunLoopState_Idle;

	// Issue memory barrier to ensure visibility of write to runLoop.selfPipeFileDescriptor1 on signal handlers and
	// other threads.
	//__atomic_signal_fence(__ATOMIC_SEQ_CST);
	//__atomic_thread_fence(__ATOMIC_SEQ_CST);
}

static void ProcessExpiredTimers(void)
{
	// Get current time.
	HAPTime now = HAPPlatformClockGetCurrent();

	// Enumerate timers.
	while (runLoop.timers) {
		if (runLoop.timers->deadline > now) {
			break;
		}

		// Update head, so that reentrant add / removes do not interfere.
		HAPPlatformTimer* expiredTimer = runLoop.timers;
		runLoop.timers = runLoop.timers->nextTimer;

		// Invoke callback.
		expiredTimer->callback((HAPPlatformTimerRef) expiredTimer, expiredTimer->context);

		// Free memory.
		os_free(expiredTimer);
	}
}

static void ProcessSelectedFileHandles(
		fd_set* readFileDescriptors,
		fd_set* writeFileDescriptors,
		fd_set* errorFileDescriptors)
{
	HAPPrecondition(readFileDescriptors);
	HAPPrecondition(writeFileDescriptors);
	HAPPrecondition(errorFileDescriptors);

	runLoop.fileHandleCursor = runLoop.fileHandles->nextFileHandle;
	while (runLoop.fileHandleCursor != runLoop.fileHandles) {
		HAPPlatformFileHandle* fileHandle = runLoop.fileHandleCursor;
		HAPAssert(fileHandle); // Note that once initialized, linked list doesn't have null pointers.
		runLoop.fileHandleCursor = fileHandle->nextFileHandle;

		if (fileHandle->isAwaitingEvents) {
			HAPAssert(fileHandle->fileDescriptor != -1);
			fileHandle->isAwaitingEvents = false;
			if (fileHandle->callback) {
				HAPPlatformFileHandleEvent fileHandleEvents;
				fileHandleEvents.isReadyForReading = fileHandle->interests.isReadyForReading &&
													FD_ISSET(fileHandle->fileDescriptor, readFileDescriptors);
				fileHandleEvents.isReadyForWriting = fileHandle->interests.isReadyForWriting &&
													FD_ISSET(fileHandle->fileDescriptor, writeFileDescriptors);
				fileHandleEvents.hasErrorConditionPending = fileHandle->interests.hasErrorConditionPending &&
															FD_ISSET(fileHandle->fileDescriptor, errorFileDescriptors);

				if (fileHandleEvents.isReadyForReading || fileHandleEvents.isReadyForWriting ||
					fileHandleEvents.hasErrorConditionPending) {
					fileHandle->callback((HAPPlatformFileHandleRef) fileHandle, fileHandleEvents, fileHandle->context);
				}
			}
		}
	}
}

void HAPPlatformRunLoopRun(void)
{
	HAPPrecondition(runLoop.state == kHAPPlatformRunLoopState_Idle);

	HAPLogInfo(&logObject, "Entering run loop.");
	runLoop.state = kHAPPlatformRunLoopState_Running;
	do {
		fd_set readFileDescriptors;
		fd_set writeFileDescriptors;
		fd_set errorFileDescriptors;

		FD_ZERO(&readFileDescriptors);
		FD_ZERO(&writeFileDescriptors);
		FD_ZERO(&errorFileDescriptors);

		int maxFileDescriptor = -1;

		HAPPlatformFileHandle* fileHandle = runLoop.fileHandles->nextFileHandle;
		while (fileHandle != runLoop.fileHandles) {
			fileHandle->isAwaitingEvents = false;
			if (fileHandle->fileDescriptor != -1) {
				if (fileHandle->interests.isReadyForReading) {
					HAPAssert(fileHandle->fileDescriptor >= 0);
					HAPAssert(fileHandle->fileDescriptor < FD_SETSIZE);
					FD_SET(fileHandle->fileDescriptor, &readFileDescriptors);
					if (fileHandle->fileDescriptor > maxFileDescriptor) {
						maxFileDescriptor = fileHandle->fileDescriptor;
					}
					fileHandle->isAwaitingEvents = true;
				}
				if (fileHandle->interests.isReadyForWriting) {
					HAPAssert(fileHandle->fileDescriptor >= 0);
					HAPAssert(fileHandle->fileDescriptor < FD_SETSIZE);
					FD_SET(fileHandle->fileDescriptor, &writeFileDescriptors);
					if (fileHandle->fileDescriptor > maxFileDescriptor) {
						maxFileDescriptor = fileHandle->fileDescriptor;
					}
					fileHandle->isAwaitingEvents = true;
				}
				if (fileHandle->interests.hasErrorConditionPending) {
					HAPAssert(fileHandle->fileDescriptor >= 0);
					HAPAssert(fileHandle->fileDescriptor < FD_SETSIZE);
					FD_SET(fileHandle->fileDescriptor, &errorFileDescriptors);
					if (fileHandle->fileDescriptor > maxFileDescriptor) {
						maxFileDescriptor = fileHandle->fileDescriptor;
					}
					fileHandle->isAwaitingEvents = true;
				}
			}
			fileHandle = fileHandle->nextFileHandle;
		}

		struct timeval timeoutValue;
		struct timeval* timeout = NULL;

		HAPTime nextDeadline = runLoop.timers ? runLoop.timers->deadline : 0;
		if (nextDeadline) {
			HAPTime now = HAPPlatformClockGetCurrent();
			HAPTime delta;
			if (nextDeadline > now) {
				delta = nextDeadline - now;
			} else {
				delta = 0;
			}
			HAPAssert(!timeout);
			timeout = &timeoutValue;
			timeout->tv_sec = (time_t)(delta / 1000);
			timeout->tv_usec = (suseconds_t)((delta % 1000) * 1000);
		}

		HAPAssert(maxFileDescriptor >= -1);
		HAPAssert(maxFileDescriptor < FD_SETSIZE);

		int e = select(
				maxFileDescriptor + 1, &readFileDescriptors, &writeFileDescriptors, &errorFileDescriptors, timeout);
		if (e == -1 && errno == EINTR) {
			continue;
		}
		if (e < 0) {
			int _errno = errno;
			HAPAssert(e == -1);
			HAPPlatformLogPOSIXError(
					kHAPLogType_Error, "System call 'select' failed.", _errno, __func__, HAP_FILE, __LINE__);
			HAPFatalError();
		}

		ProcessExpiredTimers();

		ProcessSelectedFileHandles(&readFileDescriptors, &writeFileDescriptors, &errorFileDescriptors);
	} while (runLoop.state == kHAPPlatformRunLoopState_Running);

	HAPLogInfo(&logObject, "Exiting run loop.");
	HAPAssert(runLoop.state == kHAPPlatformRunLoopState_Stopping);
	runLoop.state = kHAPPlatformRunLoopState_Idle;
}

void HAPPlatformRunLoopStop(void) {
	if (runLoop.state == kHAPPlatformRunLoopState_Running) {
		runLoop.state = kHAPPlatformRunLoopState_Stopping;
	}
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformRunLoopScheduleCallback(
		HAPPlatformRunLoopCallback callback,
		void* _Nullable context,
		size_t contextSize)
{
	HAPPrecondition(callback);
	HAPPrecondition(!contextSize || context);

	if (contextSize > UINT8_MAX) {
		HAPLogError(&logObject, "Contexts larger than UINT8_MAX are not supported.");
		return kHAPError_OutOfResources;
	}

	// Serialize event context.
	// Format: Callback pointer followed by 1 byte context size and context data.
	// Context is copied to offset 0 when invoking the callback to ensure proper alignment.
	uint8_t bytes[sizeof callback + 1 + UINT8_MAX];
	size_t numBytes = 0;
	HAPRawBufferCopyBytes(&bytes[numBytes], &callback, sizeof callback);
	numBytes += sizeof callback;
	bytes[numBytes] = (uint8_t) contextSize;
	numBytes++;
	if (context) {
		HAPRawBufferCopyBytes(&bytes[numBytes], context, contextSize);
		numBytes += contextSize;
	}
	HAPAssert(numBytes <= sizeof bytes);

	int fileDescriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fileDescriptor < 0) {
		int _errno = errno;
		HAPPlatformLogPOSIXError(kHAPLogType_Error,
			"Loopback client socket failed (log, call 'bind').",
			_errno, __func__, HAP_FILE, __LINE__);
		HAPFatalError();
	}

	int e = fcntl(fileDescriptor, F_SETFL, O_NONBLOCK);
	if (e == -1) {
		HAPPlatformLogPOSIXError(kHAPLogType_Error,
			"System call 'fcntl' to set loopback send file descriptor flags to 'non-blocking' failed.",
			errno, __func__, HAP_FILE, __LINE__);
		HAPFatalError();
	}
	
	struct sockaddr_in to_addr;
	to_addr.sin_family = AF_INET;
	to_addr.sin_port = htons(LOOPBACK_PORT);
	inet_aton("127.0.0.1", &to_addr.sin_addr);
	size_t n;
	do {
		n = sendto(fileDescriptor, bytes, numBytes, 0, (struct sockaddr *)&to_addr, sizeof(to_addr));
	} while (n == -1 && errno == EINTR);
	close(fileDescriptor);
	if (n == -1) {
		int _errno = errno;
		HAPPlatformLogPOSIXError(kHAPLogType_Error,
			"Loopback client socket failed to send data (log, call 'sendto').",
			_errno, __func__, HAP_FILE, __LINE__);
		return kHAPError_Unknown;
	}

	return kHAPError_None;
}


