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

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "HAPPlatform+Init.h"
#include "HAPPlatformFileManager.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "FileManager" };

HAP_RESULT_USE_CHECK
static HAPError
        GetRelativePath(const char* filePath, char* targetDirPath, const char* _Nonnull* _Nullable relativeFilePath) {
    HAPPrecondition(filePath);
    HAPPrecondition(targetDirPath);
    HAPPrecondition(relativeFilePath);

    // Get split relative file path and dir path
    *relativeFilePath = filePath;
    const size_t filePathLength = HAPStringGetNumBytes(filePath);
    HAPPrecondition(filePathLength);

    size_t dirPathLength = 0;
    for (size_t i = 0; i < filePathLength; ++i) {
        if (filePath[i] == '/') {
            dirPathLength = i;
            *relativeFilePath = &filePath[i + 1];
        }
    }

    HAPAssert(dirPathLength < filePathLength);
    if (dirPathLength >= filePathLength) {
        HAPLogError(
                &logObject,
                "%s failed for %s. Calculated directory length(%zu) must be smaller than file path length(%zu)",
                __func__,
                filePath,
                dirPathLength,
                filePathLength);
        return kHAPError_Unknown;
    }
    targetDirPath[dirPathLength] = '\0';

    size_t targetDirPathLength = HAPStringGetNumBytes(targetDirPath);
    HAPAssert(dirPathLength == targetDirPathLength);
    if (dirPathLength != targetDirPathLength) {
        HAPLogError(
                &logObject,
                "%s failed for %s. Calculated directory length(%zu) does not equal actual directory length(%zu).",
                __func__,
                filePath,
                dirPathLength,
                targetDirPathLength);
        return kHAPError_Unknown;
    }

    size_t reassembledPathLength = dirPathLength + HAPStringGetNumBytes(*relativeFilePath) + 1;
    HAPAssert(reassembledPathLength == filePathLength);
    if (reassembledPathLength != filePathLength) {
        HAPLogError(
                &logObject,
                "%s failed for %s. Reassembled path length(%zu) does not equal original file path length(%zu).",
                __func__,
                filePath,
                reassembledPathLength,
                filePathLength);
        return kHAPError_Unknown;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError OpenFileDescriptor(int targetDirFD, const char* filePath, int* filePathFD, int flags, int mode) {
    HAPPrecondition(filePath);

    do {
        *filePathFD = openat(targetDirFD, filePath, flags, mode);
    } while (*filePathFD == -1 && errno == EINTR);
    if (*filePathFD < 0) {
        int _errno = errno;
        HAPAssert(*filePathFD == -1);
        HAPLogError(&logObject, "open %s failed: %d.", filePath, _errno);
        return kHAPError_Unknown;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError
        WriteFileDescriptor(const char* filePath, int filePathFD, const void* _Nullable bytes, size_t numBytes) {
    HAPPrecondition(filePath);

    if (bytes) {
        size_t offset = 0;
        while (offset < numBytes) {
            // Calculate the amount of bytes to write for one loop iteration.
            size_t bytesToWrite = numBytes - offset;
            if (bytesToWrite > SSIZE_MAX) {
                bytesToWrite = SSIZE_MAX;
            }

            // Write the bytes to the file.
            ssize_t bytesWritten;
            do {
                bytesWritten = write(filePathFD, &((const uint8_t*) bytes)[offset], bytesToWrite);
            } while (bytesWritten == -1 && errno == EINTR);

            // Confirm that bytes were written to the file.
            if (bytesWritten < 0) {
                int _errno = errno;
                HAPAssert(bytesWritten == -1);
                HAPLogError(&logObject, "write to file %s failed: %d.", filePath, _errno);
                break;
            }

            if (bytesWritten == 0) {
                HAPLogError(&logObject, "write to file %s returned EOF.", filePath);
                break;
            }
            HAPAssert((size_t) bytesWritten <= bytesToWrite);
            offset += (size_t) bytesWritten;
        }
        if (offset != numBytes) {
            (void) close(filePathFD);
            int error = remove(filePath);
            if (error) {
                int _errno = errno;
                HAPAssert(error == -1);
                HAPLogError(&logObject, "removal of file %s failed: %d.", filePath, _errno);
                return kHAPError_Unknown;
            }
            HAPLogError(&logObject, "Error writing file %s.", filePath);
            return kHAPError_Unknown;
        }
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError SyncCloseFileDescriptor(int filePathFD) {
    HAPError err = kHAPError_None;
    // Try to synchronize and close the file.
    int fsyncError;
    do {
        fsyncError = fsync(filePathFD);
    } while (fsyncError == -1 && errno == EINTR);
    if (fsyncError) {
        int _errno = errno;
        HAPAssert(fsyncError == -1);
        HAPLogError(&logObject, "fsync of file id %d failed: %d.", filePathFD, _errno);
        err = kHAPError_Unknown;
    }
    (void) close(filePathFD);
    return err;
}

HAP_RESULT_USE_CHECK
static HAPError SyncDirectory(int targetDirFD) {
    // Fsync dir
    int fsyncError;
    do {
        fsyncError = fsync(targetDirFD);
    } while (fsyncError == -1 && errno == EINTR);
    if (fsyncError < 0) {
        int _errno = errno;
        HAPAssert(fsyncError == -1);
        HAPLogError(&logObject, "fsync of the target directory descriptor %d failed: %d", targetDirFD, _errno);
        return kHAPError_Unknown;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerCreateDirectory(const char* dirPath) {
    HAPPrecondition(dirPath);

    HAPError err;
    char path[PATH_MAX];

    // Duplicate string, as each path component needs to be modified to be NULL-terminated.
    // Duplicate is necessary, as input may reside in read-only memory.
    err = HAPStringWithFormat(path, sizeof path, "%s", dirPath);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&logObject, "Directory path too long: %s", dirPath);
        return kHAPError_Unknown;
    }

    // Create parent directories.
    for (char *start = path, *end = strchr(start, '/'); end; start = end + 1, end = strchr(start, '/')) {
        if (start == end) {
            // Root, or double separator.
            continue;
        }
        // Replace separator with \0 temporarily. Create directory. Restore back.
        *end = '\0';
        int error = mkdir(path, S_IRWXU);
        *end = '/';
        if (error) {
            int _errno = errno;
            HAPAssert(error == -1);
            if (_errno == EEXIST) {
                continue;
            }
            *end = '\0';
            HAPLogError(&logObject, "mkdir %s failed: %d.", path, _errno);
            *end = '/';
            return kHAPError_Unknown;
        }
    }
    // Create directory.
    int error = mkdir(path, S_IRWXU);
    if (error) {
        int _errno = errno;
        HAPAssert(error == -1);
        if (_errno != EEXIST) {
            HAPLogError(&logObject, "mkdir %s failed: %d.", path, _errno);
            return kHAPError_Unknown;
        }
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError OpenDirectory(const char* targetDirPath, DIR* _Nonnull* _Nullable targetDirRef, int* targetDirFD) {
    HAPPrecondition(targetDirPath);
    HAPPrecondition(targetDirRef);
    HAPPrecondition(targetDirFD);

    *targetDirRef = opendir(targetDirPath);
    if (!*targetDirRef) {
        int _errno = errno;
        HAPLogError(&logObject, "opendir %s failed: %d.", targetDirPath, _errno);
        return kHAPError_Unknown;
    }
    *targetDirFD = dirfd(*targetDirRef);
    if (*targetDirFD < 0) {
        int _errno = errno;
        HAPAssert(*targetDirFD == -1);
        HAPLogError(&logObject, "dirfd %s failed: %d.", targetDirPath, _errno);
        return kHAPError_Unknown;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerWriteFile(const char* filePath, const void* _Nullable bytes, size_t numBytes)
        HAP_DIAGNOSE_ERROR(!bytes && numBytes, "empty buffer cannot have a length") {
    HAPPrecondition(filePath);
    HAPAssert(bytes || numBytes); // bytes ==> numBytes > 0.

    HAPError err;

    char targetDirPath[PATH_MAX];
    err = HAPStringWithFormat(targetDirPath, sizeof targetDirPath, "%s", filePath);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&logObject, "Not enough resources to copy string: %s", filePath);
        return kHAPError_Unknown;
    }

    // Get split relative file path and dir path
    const char* relativeFilePath;
    err = GetRelativePath(filePath, &targetDirPath[0], &relativeFilePath);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Get relative path of %s failed", filePath);
        return kHAPError_Unknown;
    }

    // Create directory.
    err = HAPPlatformFileManagerCreateDirectory(targetDirPath);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Create directory %s failed.", targetDirPath);
        return kHAPError_Unknown;
    }

    // Open the target directory.
    DIR* targetDirRef;
    int targetDirFD;
    err = OpenDirectory(targetDirPath, &targetDirRef, &targetDirFD);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Open directory %s failed.", targetDirPath);
        if (targetDirRef) {
            HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        }
        return kHAPError_Unknown;
    }

    // Create the filename of the temporary file.
    char tmpPath[PATH_MAX];
    err = HAPStringWithFormat(tmpPath, sizeof tmpPath, "%s-tmp", relativeFilePath);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&logObject, "Not enough resources to get path: %s-tmp", relativeFilePath);
        HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        return kHAPError_Unknown;
    }

    // Open the tempfile
    int tmpPathFD;
    int writeFlags = O_CREAT | O_WRONLY | O_TRUNC;
    int writeMode = S_IRUSR;
    err = OpenFileDescriptor(targetDirFD, &tmpPath[0], &tmpPathFD, writeFlags, writeMode);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "open %s in %s failed.", tmpPath, targetDirPath);
        HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        return kHAPError_Unknown;
    }

    // Write to the temporary file
    err = WriteFileDescriptor(tmpPath, tmpPathFD, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Write to temporary file %s in %s failed.", tmpPath, targetDirPath);
        HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        return kHAPError_Unknown;
    }

    // Try to synchronize and close the temporary file.
    err = SyncCloseFileDescriptor(tmpPathFD);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Sync and Close of temporary file %s failed.", tmpPath);
    }

    // Sync directory
    err = SyncDirectory(targetDirFD);
    if (err) {

        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Sync of the target directory %s failed", targetDirPath);
        HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        return kHAPError_Unknown;
    }

    // Rename file
    int e = renameat(targetDirFD, tmpPath, targetDirFD, relativeFilePath);
    if (e) {
        int _errno = errno;
        HAPAssert(e == -1);
        HAPLogError(&logObject, "rename of temporary file %s to %s failed: %d.", tmpPath, filePath, _errno);
        HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        return kHAPError_Unknown;
    }

    // Fsync dir
    err = SyncDirectory(targetDirFD);
    if (err) {

        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Sync of the target directory %s failed", targetDirPath);
        HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        return kHAPError_Unknown;
    }

    HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerAppendFile(const char* filePath, const void* bytes, size_t numBytes) {
    HAPPrecondition(filePath);
    HAPPrecondition(bytes);
    HAPAssert(numBytes > 0);

    HAPError err;

    char targetDirPath[PATH_MAX];
    err = HAPStringWithFormat(targetDirPath, sizeof targetDirPath, "%s", filePath);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&logObject, "Not enough resources to copy string: %s", filePath);
        return kHAPError_Unknown;
    }

    // Get split relative file path and dir path
    const char* relativeFilePath;
    err = GetRelativePath(filePath, &targetDirPath[0], &relativeFilePath);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Get relative path of %s failed", filePath);
        return kHAPError_Unknown;
    }

    // Create directory.
    err = HAPPlatformFileManagerCreateDirectory(targetDirPath);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Create directory %s failed.", targetDirPath);
        return kHAPError_Unknown;
    }

    // Open the target directory.
    DIR* targetDirRef;
    int targetDirFD;
    err = OpenDirectory(targetDirPath, &targetDirRef, &targetDirFD);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Open directory %s failed.", targetDirPath);
        if (targetDirRef) {
            HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        }
        return kHAPError_Unknown;
    }

    // Open the file
    int relativePathFD;
    int writeFlags = O_CREAT | O_WRONLY | O_APPEND;
    int writeMode = S_IRUSR | S_IWUSR;
    err = OpenFileDescriptor(targetDirFD, &relativeFilePath[0], &relativePathFD, writeFlags, writeMode);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "open %s in %s failed.", relativeFilePath, targetDirPath);
        HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        return kHAPError_Unknown;
    }

    // Append to the temporary file
    err = WriteFileDescriptor(relativeFilePath, relativePathFD, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Append to file %s in %s failed.", relativeFilePath, targetDirPath);
        HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        return kHAPError_Unknown;
    }

    // Try to synchronize and close the temporary file.
    err = SyncCloseFileDescriptor(relativePathFD);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Sync and Close of file %s failed.", relativeFilePath);
    }

    // Sync directory
    err = SyncDirectory(targetDirFD);
    if (err) {

        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Sync of the target directory %s failed", targetDirPath);
        HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        return kHAPError_Unknown;
    }
    HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerReadFile(
        const char* filePath,
        void* _Nullable bytes,
        size_t maxBytes,
        size_t* _Nullable numBytes,
        bool* valid) {
    HAPPrecondition(filePath);
    HAPPrecondition(!maxBytes || bytes);
    HAPPrecondition((bytes == NULL) == (numBytes == NULL));
    HAPPrecondition(valid);

    *valid = false;

    int fd;
    do {
        fd = open(filePath, O_RDONLY);
    } while (fd == -1 && errno == EINTR);

    if (fd < 0) {
        int _errno = errno;
        if (_errno == ENOENT) {
            return kHAPError_None;
        }
        HAPAssert(fd == -1);
        HAPLogError(&logObject, "open %s failed: %d.", filePath, _errno);
        return kHAPError_Unknown;
    }
    *valid = true;

    if (bytes) {
        HAPAssert(numBytes);

        size_t offset = 0;
        while (offset < maxBytes) {
            size_t bytesToRead = maxBytes - offset;
            if (bytesToRead > SSIZE_MAX) {
                bytesToRead = SSIZE_MAX;
            }

            ssize_t bytesRead;
            do {
                bytesRead = read(fd, &((uint8_t*) bytes)[offset], bytesToRead);
            } while (bytesRead == -1 && errno == EINTR);
            if (bytesRead < 0) {
                int _errno = errno;
                HAPAssert(bytesRead == -1);
                HAPLogError(&logObject, "read %s failed: %d.", filePath, _errno);
                (void) close(fd);
                return kHAPError_Unknown;
            }

            HAPAssert((size_t) bytesRead <= bytesToRead);
            offset += (size_t) bytesRead;

            if (bytesRead == 0) {
                break;
            }
        }
        *numBytes = offset;
    }
    (void) close(fd);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerRemoveFile(const char* filePath) {
    HAPPrecondition(filePath);

    HAPError err;

    // Test for a regular file or a symbolic link.
    {
        struct stat statBuffer;
        int e = stat(filePath, &statBuffer);
        if (e) {
            int _errno = errno;
            HAPAssert(e == -1);
            if (_errno == ENOENT) {
                // File does not exist.
                return kHAPError_None;
            }
            HAPLogError(&logObject, "stat file %s failed: %d.", filePath, _errno);
            HAPFatalError();
        }
        if (!S_ISREG(statBuffer.st_mode) && !S_ISLNK(statBuffer.st_mode)) {
            HAPLogError(&logObject, "file %s is neither a regular file nor a symbolic link.", filePath);
            HAPFatalError();
        }
    }

    // Remove file.
    {
        int e = unlink(filePath);
        if (e) {
            int _errno = errno;
            HAPAssert(e == -1);
            if (_errno == ENOENT) {
                // File does not exist.
                return kHAPError_None;
            }
            HAPLogError(&logObject, "unlink file %s failed: %d.", filePath, _errno);
            return kHAPError_Unknown;
        }
    }

    // Try to synchronize the directory containing the removed file.
    {
        char targetDirPath[PATH_MAX];
        err = HAPStringWithFormat(targetDirPath, sizeof targetDirPath, "%s", filePath);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLogError(&logObject, "Not enough resources to copy string: %s", filePath);
            return kHAPError_None;
        }
        char* lastSeparator = NULL;
        for (char *start = targetDirPath, *end = strchr(start, '/'); end; start = end + 1, end = strchr(start, '/')) {
            lastSeparator = end;
        }
        if (!lastSeparator) {
            // No directory in path - treat as current working directory.
            HAPAssert(sizeof targetDirPath > 2);
            targetDirPath[0] = '.';
            targetDirPath[1] = '\0';
        } else {
            // Replace final separator with '\0' to cut off the file name.
            HAPAssert(lastSeparator < &targetDirPath[sizeof targetDirPath - 1]);
            HAPAssert(*lastSeparator == '/');
            HAPAssert(*(lastSeparator + 1));
            *lastSeparator = '\0';
        }

        int targetDirFd;
        do {
            targetDirFd = open(targetDirPath, O_RDONLY);
        } while (targetDirFd == -1 && errno == EINTR);
        if (targetDirFd < 0) {
            int _errno = errno;
            HAPAssert(targetDirFd == -1);
            HAPLogError(&logObject, "open target directory %s failed: %d.", targetDirPath, _errno);
            return kHAPError_None;
        }

        int e;
        do {
            e = fsync(targetDirFd);
        } while (e == -1 && errno == EINTR);
        if (e) {
            int _errno = errno;
            HAPAssert(e == -1);
            HAPLogError(&logObject, "fsync of target directory file %s failed: %d.", targetDirPath, _errno);
        }
        (void) close(targetDirFd);
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerGetFileSize(const char* filePath, off_t* fileSize) {
    HAPPrecondition(filePath);
    HAPPrecondition(fileSize);

    struct stat statBuffer;
    int e = stat(filePath, &statBuffer);
    if (e) {
        int _errno = errno;
        HAPAssert(e == -1);
        if (_errno == ENOENT) {
            // File does not exist.
            *fileSize = 0;
            return kHAPError_None;
        }
        HAPLogError(&logObject, "stat file %s failed: %d.", filePath, _errno);
        HAPFatalError();
        return kHAPError_Unknown;
    }

    *fileSize = statBuffer.st_size;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerFilterTextFile(
        const char* filePath,
        HAPPlatformFileManagerFilterTextFileCallback callback,
        void* _Nullable context) {
    HAPPrecondition(filePath);
    HAPPrecondition(callback);

    HAPError err;

    char targetDirPath[PATH_MAX];
    err = HAPStringWithFormat(targetDirPath, sizeof targetDirPath, "%s", filePath);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&logObject, "Not enough resources to copy string: %s", filePath);
        return kHAPError_Unknown;
    }

    // Get split relative file path and dir path
    const char* relativeFilePath;
    err = GetRelativePath(filePath, &targetDirPath[0], &relativeFilePath);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Get relative path of %s failed", filePath);
        return kHAPError_Unknown;
    }

    // Create directory.
    err = HAPPlatformFileManagerCreateDirectory(targetDirPath);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Create directory %s failed.", targetDirPath);
        return kHAPError_Unknown;
    }

    // Open the target directory.
    DIR* targetDirRef;
    int targetDirFD;
    err = OpenDirectory(targetDirPath, &targetDirRef, &targetDirFD);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Open directory %s failed.", targetDirPath);
        if (targetDirRef) {
            HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        }
        return kHAPError_Unknown;
    }

    // Open the file
    int relativePathFD;
    int writeFlags = O_RDWR | O_CREAT;
    int writeMode = S_IRUSR | S_IWUSR;
    err = OpenFileDescriptor(targetDirFD, &relativeFilePath[0], &relativePathFD, writeFlags, writeMode);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "open %s in %s failed.", relativeFilePath, targetDirPath);
        HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        return kHAPError_Unknown;
    }

    // Test for a regular file or a symbolic link.
    struct stat statBuffer;
    int e = fstat(relativePathFD, &statBuffer);
    if (e) {
        int _errno = errno;
        HAPAssert(e == -1);
        HAPLogError(&logObject, "fstat %s in %s failed: %d.", relativeFilePath, targetDirPath, _errno);
        (void) close(relativePathFD);
        HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        return kHAPError_Unknown;
    }
    if (!S_ISREG(statBuffer.st_mode) && !S_ISLNK(statBuffer.st_mode)) {
        HAPLogError(
                &logObject,
                "File %s in %s is neither a regular file nor a symbolic link.",
                relativeFilePath,
                targetDirPath);
        (void) close(relativePathFD);
        HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        return kHAPError_Unknown;
    }

    // Perform filtering.
    if (statBuffer.st_size) {
        void* _Nullable bytes_ = mmap(
                /* addr: */ NULL, statBuffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, relativePathFD, /* off: */ 0);
        if (bytes_ == MAP_FAILED) {
            int _errno = errno;
            HAPLogError(&logObject, "mmap %s in %s failed: %d.", relativeFilePath, targetDirPath, _errno);
            (void) close(relativePathFD);
            HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
            return kHAPError_Unknown;
        }
        char* bytes = HAPNonnullVoid(bytes_);
        size_t numBytes = statBuffer.st_size;

        char* start = bytes;
        while (start < &bytes[numBytes]) {
            char* end = memchr(start, '\n', numBytes - (start - bytes));
            if (end) {
                *end = '\0';
                bool shouldInclude = callback(context, filePath, start);
                *end = '\n';
                if (!shouldInclude) {
                    HAPRawBufferCopyBytes(start, end + 1, numBytes - (end + 1 - bytes));
                    numBytes -= end + 1 - start;
                } else {
                    start = end + 1;
                    if (start == &bytes[numBytes]) {
                        // Report new line at end of file.
                        shouldInclude = callback(context, filePath, "");
                        (void) shouldInclude;
                    }
                }
            } else {
                bool shouldInclude = callback(context, filePath, start);
                if (!shouldInclude) {
                    numBytes -= &bytes[numBytes] - start;
                } else {
                    start = &bytes[numBytes];
                }
            }
        }
        HAPAssert(start == &bytes[numBytes]);

        e = munmap(bytes, statBuffer.st_size);
        if (e) {
            int _errno = errno;
            HAPAssert(e == -1);
            HAPLogError(&logObject, "munmap %s in %s failed: %d.", relativeFilePath, targetDirPath, _errno);
            (void) close(relativePathFD);
            HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
            return kHAPError_Unknown;
        }

        e = ftruncate(relativePathFD, numBytes);
        if (e) {
            int _errno = errno;
            HAPAssert(e == -1);
            HAPLogError(&logObject, "ftruncate %s in %s failed: %d.", relativeFilePath, targetDirPath, _errno);
            (void) close(relativePathFD);
            HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
            return kHAPError_Unknown;
        }
    }

    // Try to synchronize and close the file.
    err = SyncCloseFileDescriptor(relativePathFD);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Sync and Close of file %s failed.", relativeFilePath);
    }

    // Sync directory
    err = SyncDirectory(targetDirFD);
    if (err) {

        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Sync of the target directory %s failed", targetDirPath);
        HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
        return kHAPError_Unknown;
    }
    HAPPlatformFileManagerCloseDirFreeSafe(targetDirRef);
    return kHAPError_None;
}

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif
