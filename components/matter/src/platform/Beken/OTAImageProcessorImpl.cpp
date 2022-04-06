/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <app/clusters/ota-requestor/OTADownloader.h>
#include <lib/support/logging/CHIPLogging.h>

#include <platform/Beken/OTAImageProcessorImpl.h>
#include <string.h>
#include "matter_pal.h"


namespace chip {

const char ucFinishFlag[] = {0xF0,0x5D,0x4A,0x8C};//Flag that OTA update has finished 
CHIP_ERROR OTAImageProcessorImpl::PrepareDownload()
{
    ChipLogProgress(SoftwareUpdate, "Prepare download");

    DeviceLayer::PlatformMgr().ScheduleWork(HandlePrepareDownload, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::Finalize()
{
    ChipLogProgress(SoftwareUpdate, "Finalize");

    DeviceLayer::PlatformMgr().ScheduleWork(HandleFinalize, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::Apply()
{
    ChipLogProgress(SoftwareUpdate, "Apply");

    DeviceLayer::PlatformMgr().ScheduleWork(HandleApply, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::Abort()
{
    ChipLogProgress(SoftwareUpdate, "Abort");

    DeviceLayer::PlatformMgr().ScheduleWork(HandleAbort, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::ProcessBlock(ByteSpan & block)
{
    ChipLogProgress(SoftwareUpdate, "Process Block");

    if ((block.data() == nullptr) || block.empty())
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    CHIP_ERROR err = SetBlock(block);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(SoftwareUpdate, "Cannot set block data: %" CHIP_ERROR_FORMAT, err.Format());
        return err;
    }

    DeviceLayer::PlatformMgr().ScheduleWork(HandleProcessBlock, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

void OTAImageProcessorImpl::HandlePrepareDownload(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    if (imageProcessor == nullptr)
    {
        ChipLogError(SoftwareUpdate, "ImageProcessor context is null");
        return;
    }
    else if (imageProcessor->mDownloader == nullptr)
    {
        ChipLogError(SoftwareUpdate, "mDownloader is null");
        return;
    }

    ChipLogProgress(SoftwareUpdate, "%s [%d] OTA address space will be upgraded",__FUNCTION__,__LINE__);

    imageProcessor->mDownloader->OnPreparedForDownload(CHIP_NO_ERROR);
}

void OTAImageProcessorImpl::HandleFinalize(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    bk_logic_partition_t *partition_info = NULL;
    UINT32 dwFlagAddrOffset = 0;
    if (imageProcessor == nullptr)
    {
        ChipLogError(SoftwareUpdate, "ImageProcessor context is null");
        return;
    }
    
    partition_info = bk_flash_get_info(BK_PARTITION_OTA);
    BK_CHECK_POINTER_NULL_TO_VOID(partition_info);
    dwFlagAddrOffset =  partition_info->partition_length - (sizeof(ucFinishFlag) - 1);
    
    bk_write_ota_data_to_flash((char *)ucFinishFlag,dwFlagAddrOffset,(sizeof(ucFinishFlag) - 1));

    imageProcessor->ReleaseBlock();

    ChipLogProgress(SoftwareUpdate, "OTA image downloaded and written to flash");
}

void OTAImageProcessorImpl::HandleAbort(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    if (imageProcessor == nullptr)
    {
        ChipLogError(SoftwareUpdate, "ImageProcessor context is null");
        return;
    }

    // Abort OTA procedure

    imageProcessor->ReleaseBlock();
}

void OTAImageProcessorImpl::HandleProcessBlock(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    
    if (imageProcessor == nullptr)
    {
        ChipLogError(SoftwareUpdate, "ImageProcessor context is null");
        return;
    }
    else if (imageProcessor->mDownloader == nullptr)
    {
        ChipLogError(SoftwareUpdate, "mDownloader is null");
        return;
    }

    if (!imageProcessor->readHeader) // First block received, process header
    {
        ota_data_struct_t * tempBuf = (ota_data_struct_t *) chip::Platform::MemoryAlloc(sizeof(ota_data_struct_t));
        
        if(NULL == tempBuf)
        {
            ChipLogError(SoftwareUpdate, "%s [%d] malloc failed  ",__FUNCTION__,__LINE__);
        }
        memset((char *)tempBuf,0,sizeof(ota_data_struct_t));
        memcpy((char *)&(imageProcessor->pOtaTgtHdr), imageProcessor->mBlock.data(), sizeof(ota_data_struct_t));

        imageProcessor->flash_data_offset = 0;

        bk_read_ota_data_in_flash( (char *)tempBuf,imageProcessor->flash_data_offset,sizeof(ota_data_struct_t));
        ChipLogProgress(SoftwareUpdate, "Download version %s,date is %ld ",tempBuf->version,tempBuf->timestamp);
        ChipLogProgress(SoftwareUpdate, "Download size_raw %ld,size_package is %ld ",tempBuf->size_raw,tempBuf->size_package);
        ChipLogProgress(SoftwareUpdate, "imageProcessor version %s,date is 0x%lx ",imageProcessor->pOtaTgtHdr.version,imageProcessor->pOtaTgtHdr.timestamp);

        bk_logic_partition_t *partition_info = NULL;
        UINT32 dwFlagAddrOffset = 0;
        char ucflag[(sizeof(ucFinishFlag) - 1)] = {0};
        
        partition_info = bk_flash_get_info(BK_PARTITION_OTA);
        BK_CHECK_POINTER_NULL_TO_VOID(partition_info);

        dwFlagAddrOffset =  partition_info->partition_length - (sizeof(ucFinishFlag) - 1);
        bk_read_ota_data_in_flash( (char *)ucflag,dwFlagAddrOffset,(sizeof(ucFinishFlag) - 1));
        ChipLogProgress(SoftwareUpdate, "Block size is %d ,ucFinishFlag size len is %d",imageProcessor->mBlock.size(),sizeof(ucFinishFlag));

        if ((0 == memcmp(ucflag,ucFinishFlag,(sizeof(ucFinishFlag) - 1))) && 
            (0 == memcmp(tempBuf->version,imageProcessor->pOtaTgtHdr.version,sizeof(imageProcessor->pOtaTgtHdr.version))))
        {
            chip::Platform::MemoryFree(tempBuf);
            tempBuf = NULL;
            ChipLogError(SoftwareUpdate, "The version is is the same as the previous version");
            return;
        }

        imageProcessor->readHeader = true;
        ChipLogProgress(SoftwareUpdate, "flash_data_offset is 0x%lx",imageProcessor->flash_data_offset);

        // Erase update partition
        ChipLogProgress(SoftwareUpdate, "Erasing target partition...");
        bk_erase_ota_data_in_flash();
        ChipLogProgress(SoftwareUpdate, "Erasing target partition...");
        if(0 != bk_write_ota_data_to_flash((char *) imageProcessor->mBlock.data(),imageProcessor->flash_data_offset,imageProcessor->mBlock.size()))
        {
            chip::Platform::MemoryFree(tempBuf);
            tempBuf = NULL;
            ChipLogError(SoftwareUpdate, "bk_write_ota_data_to_flash failed %s [%d] ",__FUNCTION__,__LINE__);
            imageProcessor->mDownloader->EndDownload(CHIP_ERROR_WRITE_FAILED);
            return;
        }
        imageProcessor->flash_data_offset += imageProcessor->mBlock.size();//count next write flash address

        chip::Platform::MemoryFree(tempBuf);
        tempBuf = NULL;
    }
    else // received subsequent blocks
    {
        if(0 != bk_write_ota_data_to_flash( (char *)imageProcessor->mBlock.data(),imageProcessor->flash_data_offset,imageProcessor->mBlock.size()))
        {
            ChipLogError(SoftwareUpdate, "bk_write_ota_data_to_flash failed %s [%d] ",__FUNCTION__,__LINE__);
            imageProcessor->mDownloader->EndDownload(CHIP_ERROR_WRITE_FAILED);
            return;
        }
        imageProcessor->flash_data_offset += imageProcessor->mBlock.size();//count next write flash address

        imageProcessor->size += imageProcessor->mBlock.size();
    }

    imageProcessor->mParams.downloadedBytes += imageProcessor->mBlock.size();
    imageProcessor->mDownloader->FetchNextData();
}

void OTAImageProcessorImpl::HandleApply(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    ChipLogError(SoftwareUpdate, "Update completly,will reboot %s [%d] ",__FUNCTION__,__LINE__);

    // Reboot
    bk_reboot();
}

CHIP_ERROR OTAImageProcessorImpl::SetBlock(ByteSpan & block)
{
    if ((block.data() == nullptr) || block.empty())
    {
        return CHIP_NO_ERROR;
    }

    if (mBlock.size() < block.size())
    {
        if (!mBlock.empty())
        {
            ReleaseBlock();
        }
        uint8_t * mBlock_ptr = static_cast<uint8_t *>(chip::Platform::MemoryAlloc(block.size()));
        if (mBlock_ptr == nullptr)
        {
            return CHIP_ERROR_NO_MEMORY;
        }
        mBlock = MutableByteSpan(mBlock_ptr, block.size());
    }

    // Allocate memory for block data if it has not been done yet
    if (mBlock.empty())
    {
        mBlock = MutableByteSpan(static_cast<uint8_t *>(chip::Platform::MemoryAlloc(block.size())), block.size());
        if (mBlock.data() == nullptr)
        {
            return CHIP_ERROR_NO_MEMORY;
        }
    }

    // Store the actual block data
    CHIP_ERROR err = CopySpanToMutableSpan(block, mBlock);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(SoftwareUpdate, "Cannot copy block data: %" CHIP_ERROR_FORMAT, err.Format());
        return err;
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::ReleaseBlock()
{
    if (mBlock.data() != nullptr)
    {
        chip::Platform::MemoryFree(mBlock.data());
    }

    mBlock = MutableByteSpan();
    return CHIP_NO_ERROR;
}
} // namespace chip
