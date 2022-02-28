/* eip130.c
 *
 * EIP-13x Root of Trust / Security Module Driver Library API implementation
 */

/*****************************************************************************
* Copyright (c) 2014-2019 INSIDE Secure B.V. All Rights Reserved.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "c_eip130.h"               // configuration

#include "basic_defs.h"             // uint8_t, IDENTIFIER_NOT_USED, etc.
#include "device_types.h"           // Device_Handle_t
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t
#include "eip130_level0.h"

#include "eip130.h"                 // the API we will implement


#ifndef EIP130_REMOVE_MODULEGETOPTIONS
/*----------------------------------------------------------------------------
 * EIP130_ModuleGetOptions
 */
void
EIP130_ModuleGetOptions(
        Device_Handle_t Device,
        uint8_t * const fFWRam_p,
        uint8_t * const BusIfc_p,
        uint8_t * const Engines_p,
        uint16_t * const CustomEngines_p)
{
    uint32_t Value = EIP130_RegisterReadOptions2(Device);

    if (fFWRam_p)
    {
        *fFWRam_p = (uint8_t)(MASK_1_BIT & (Value >> 9));
    }

    if (BusIfc_p)
    {
        *BusIfc_p = (uint8_t)(MASK_1_BIT & (Value >> 12));
    }

    if (Engines_p)
    {
        *Engines_p = (uint8_t)(MASK_6_BITS & (Value >> 0));
    }

    if (CustomEngines_p)
    {
        *CustomEngines_p = (uint16_t)(128 << (MASK_10_BITS & (Value >> 16)));
    }
}
#endif


#ifndef EIP130_REMOVE_MODULEGETSTATUS
/*----------------------------------------------------------------------------
 * EIP130_ModuleGetStatus
 */
void
EIP130_ModuleGetStatus(
        Device_Handle_t Device,
        uint8_t * const fFIPSMode_p,
        uint8_t * const fFatalError_p,
        uint8_t * const fCRC24Ok_p,
        uint8_t * const fCRC24Busy_p,
        uint8_t * const fCRC24Error_p,
        uint8_t * const fFirmwareWritten_p,
        uint8_t * const fFirmwareChecksDone_p,
        uint8_t * const fFirmwareAccepted_p)
{
    uint32_t Value = EIP130_RegisterReadModuleStatus(Device);

    if (fFIPSMode_p)
    {
        if ((Value & BIT_1) == 0)
        {
            *fFIPSMode_p = (uint8_t)(MASK_1_BIT & (Value >> 0));
        }
        else
        {
            *fFIPSMode_p = 0;
        }
    }

    if (fFatalError_p)
    {
        *fFatalError_p = (uint8_t)(MASK_1_BIT & (Value >> 31));
    }

    if (fCRC24Ok_p)
    {
        *fCRC24Ok_p = (uint8_t)(MASK_1_BIT & (Value >> 9));
    }
    if (fCRC24Busy_p)
    {
        *fCRC24Busy_p = (uint8_t)(MASK_1_BIT & (Value >> 8));
    }
    if (fCRC24Error_p)
    {
        *fCRC24Error_p = (uint8_t)(MASK_1_BIT & (Value >> 10));
    }

    if (fFirmwareWritten_p)
    {
        *fFirmwareWritten_p = (uint8_t)(MASK_1_BIT & (Value >> 20));
    }
    if (fFirmwareChecksDone_p)
    {
        *fFirmwareChecksDone_p = (uint8_t)(MASK_1_BIT & (Value >> 22));
    }
    if (fFirmwareAccepted_p)
    {
        *fFirmwareAccepted_p = (uint8_t)(MASK_1_BIT & (Value >> 23));
    }
}
#endif


#ifndef EIP130_REMOVE_MODULEFIRMWAREWRITTEN
/*----------------------------------------------------------------------------
 * EIP130_ModuleFirmwareWritten
 */
void
EIP130_ModuleFirmwareWritten(
        Device_Handle_t Device)
{
    EIP130_RegisterWriteModuleStatus(Device, EIP130_FIRMWARE_WRITTEN);
}
#endif


#ifndef EIP130_REMOVE_MAILBOXACCESSVERIFY
/*----------------------------------------------------------------------------
 * EIP130_MailboxAccessVerify
 */
int
EIP130_MailboxAccessVerify(
        Device_Handle_t Device,
        const uint8_t MailboxNr)
{
    uint32_t Value;

    // Check for the hardware (EIP-130/140 or EIP-133)
    Value = EIP130_RegisterReadVersion(Device) & 0x0000FFFF;
    if ((Value != 0x7D82) && (Value != 0x7A85) && (Value != 0x738C))
    {
        return -1;                      // Not supported (Is HW active?)
    }

    // Check if the mailbox number is valid
    Value = EIP130_RegisterReadOptions(Device);
    if (MailboxNr > (MASK_4_BITS & Value))
    {
        return -2;                      // Invalid mailbox number
    }
    // Intentionally no lock-out check here

    return 0;                           // success
}
#endif


/*----------------------------------------------------------------------------
 * EIP130_MailboxGetOptions
 */
void
EIP130_MailboxGetOptions(
        Device_Handle_t Device,
        uint8_t * const MyHostID_p,
        uint8_t * const MasterID_p,
        uint8_t * const MyProt_p,
        uint8_t * const ProtAvailable_p,
        uint8_t * const NrOfMailboxes_p,
        uint16_t * const MailboxSize_p)
{
    uint32_t Value = EIP130_RegisterReadOptions(Device);

    if (MyHostID_p)
    {
        *MyHostID_p = (uint8_t)(MASK_3_BITS & (Value >> 20));
    }
    if (MyProt_p)
    {
        *MyProt_p = (uint8_t)(MASK_1_BIT & (Value >> 23));
    }

    if (MasterID_p)
    {
        *MasterID_p = (uint8_t)(MASK_3_BITS & (Value >> 16));
    }
    if (ProtAvailable_p)
    {
        *ProtAvailable_p = (uint8_t)(MASK_1_BIT & (Value >> 19));
    }

    if (NrOfMailboxes_p)
    {
        *NrOfMailboxes_p = (uint8_t)(MASK_4_BITS & Value);
    }

    if (MailboxSize_p)
    {
        *MailboxSize_p = (uint16_t)(128 << (MASK_2_BITS & (Value >> 4)));
    }
}


#ifndef EIP130_REMOVE_MAILBOXLINKID
/*----------------------------------------------------------------------------
 * EIP130_MailboxGetLinkID
 */
int
EIP130_MailboxGetLinkID(
        Device_Handle_t Device,
        const uint8_t MailboxNr,
        uint8_t * const HostID_p,
        uint8_t * const Secure_p);
{
    uint32_t Value;

#ifdef EIP130_STRICT_ARGS
    if (MailboxNr < 1 || MailboxNr > 8)
    {
         return -1;
    }
#endif

    // Get linked state
    Value = EIP130_RegisterReadMailboxStatus(Device);
    if ((Value & (BIT_2 << ((MailboxNr - 1) * 4))) == 0)
    {
        return -2;                      // Not linked
    }

    // Get linkID's
    Value = EIP130_RegisterReadMailboxLinkId(Device);
    Value >>= ((MailboxNr - 1) * 4);

    if (HostID_p != NULL)
    {
        *HostID_p = Value & MASK_3_BITS;
    }

    if (Secure_p != NULL)
    {
        if (Value & BIT_3)
        {
            *Secure_p = 1;
        }
        else
        {
            *Secure_p = 0;
        }
    }

    return 0;                           // Success
}
#endif


#ifndef EIP130_REMOVE_MAILBOXACCESSCONTROL
/*----------------------------------------------------------------------------
 * EIP130_MailboxAccessControl
 */
int
EIP130_MailboxAccessControl(
        Device_Handle_t Device,
        uint8_t MailboxNr,
        uint8_t HostNr,
        bool fAccessAllowed)
{
    uint32_t LockOut = EIP130_RegisterReadMailboxLockout(Device);
    uint32_t BitMask = BIT_0;

#ifdef EIP130_STRICT_ARGS
    if (MailboxNr < 1 || MailboxNr > 8)
    {
         return -1;
    }

    if (HostNr > 7)
    {
         return -2;
    }
#endif

    {
        int BitNr = (MailboxNr - 1) * 8 + HostNr;
        if (BitNr > 0)
        {
             BitMask <<= BitNr;
        }
    }

    if (fAccessAllowed)
    {
        // clear a bit grant access
        LockOut &= ~BitMask;
    }
    else
    {
        // set a bit to lock out access
        LockOut |= BitMask;
    }

    EIP130_RegisterWriteMailboxLockout(Device, LockOut);

    return 0;                           // Success
}
#endif


#if !defined(EIP130_REMOVE_MAILBOXLINK) || \
    !defined(EIP130_REMOVE_FIRMWAREDOWNLOAD)
/*----------------------------------------------------------------------------
 * EIP130_MailboxLink
 */
int
EIP130_MailboxLink(
        Device_Handle_t Device,
        const uint8_t MailboxNr)
{
    uint32_t SetValue;
    uint32_t GetValue;

#ifdef EIP130_STRICT_ARGS
    if (MailboxNr < 1 || MailboxNr > 8)
    {
         return -1;
    }
#endif

    SetValue = BIT_2 << ((MailboxNr - 1) * 4);
    EIP130_RegisterWriteMailboxControl(Device, SetValue);

    GetValue = EIP130_RegisterReadMailboxStatus(Device);
    if ((GetValue & SetValue) != SetValue)
    {
        return -2;                      // Link failed
    }

    return 0;                           // Success
}
#endif


#if !defined(EIP130_REMOVE_MAILBOXUNLINK) || \
    !defined(EIP130_REMOVE_FIRMWAREDOWNLOAD)
/*----------------------------------------------------------------------------
 * EIP130_MailboxUnlink
 */
int
EIP130_MailboxUnlink(
        Device_Handle_t Device,
        const uint8_t MailboxNr)
{
    uint32_t SetValue;
    uint32_t GetValue;

#ifdef EIP130_STRICT_ARGS
    if (MailboxNr < 1 || MailboxNr > 8)
    {
         return -1;
    }
#endif

    // Unlink mailbox
    SetValue = BIT_3 << ((MailboxNr - 1) * 4);
    EIP130_RegisterWriteMailboxControl(Device, SetValue);
    SetValue >>= 1;                     // Adapt for linked check

    // Check if the mailbox is still linked
    GetValue = EIP130_RegisterReadMailboxStatus(Device);
    if ((GetValue & SetValue) != 0)
    {
        return -2;                      // Unlinking failed
    }
    return 0;                           // Success
}
#endif


#if !defined(EIP130_REMOVE_MAILBOXLINKRESET) || \
    !defined(EIP130_REMOVE_FIRMWAREDOWNLOAD)
/*----------------------------------------------------------------------------
 * EIP130_MailboxLinkReset
 */
int
EIP130_MailboxLinkReset(
        Device_Handle_t Device,
        const uint8_t MailboxNr)
{
    uint32_t SetValue;
    uint32_t GetValue;

#ifdef EIP130_STRICT_ARGS
    if (MailboxNr < 1 || MailboxNr > 8)
    {
         return -1;
    }
#endif

    SetValue = BIT_3 << ((MailboxNr - 1) * 4);
    EIP130_RegisterWriteMailboxReset(Device, SetValue);

    GetValue = EIP130_RegisterReadMailboxStatus(Device);
    if ((GetValue & SetValue) != SetValue)
    {
        return -2;                      // Link reset failed
    }

    return 0;                           // Success
}
#endif


#if !defined(EIP130_REMOVE_MAILBOXCANWRITETOKEN) || \
    !defined(EIP130_REMOVE_FIRMWAREDOWNLOAD)
/*----------------------------------------------------------------------------
 * EIP130_MailboxCanWriteToken
 */
bool
EIP130_MailboxCanWriteToken(
        Device_Handle_t Device,
        const uint8_t MailboxNr)
{
    uint32_t Value;

#ifdef EIP130_STRICT_ARGS
    if (MailboxNr < 1 || MailboxNr > 8)
    {
         return false;
    }
#endif

    Value = EIP130_RegisterReadMailboxStatus(Device);
    if ((Value & (BIT_0 << ((MailboxNr - 1) * 4))) == 0)
    {
         return true;
    }
    return false;
}
#endif


/*----------------------------------------------------------------------------
 * EIP130_MailboxCanReadToken
 */
bool
EIP130_MailboxCanReadToken(
        Device_Handle_t Device,
        const uint8_t MailboxNr)
{
    uint32_t Value;

#ifdef EIP130_STRICT_ARGS
    if (MailboxNr < 1 || MailboxNr > 8)
    {
         return false;
    }
#endif

    Value = EIP130_RegisterReadMailboxStatus(Device);
    if ((Value & (BIT_1 << ((MailboxNr - 1) * 4))) != 0)
    {
        return true;
    }
    return false;
}


/*----------------------------------------------------------------------------
 * EIP130_MailboxWriteAndSubmitToken
 */
int
EIP130_MailboxWriteAndSubmitToken(
        Device_Handle_t Device,
        const uint8_t MailboxNr,
        const Eip130Token_Command_t * const CommandToken_p)
{
#ifdef EIP130_STRICT_ARGS
    if (MailboxNr < 1 || MailboxNr > 8)
    {
         return -1;
    }

    if (CommandToken_p == NULL)
    {
         return -2;
    }
#endif

#ifndef EIP130_REMOVE_MAILBOXCANWRITETOKEN
    if (!EIP130_MailboxCanWriteToken(Device, MailboxNr))
    {
         return -3;
    }
#endif

    // Copy the token to the IN mailbox
    {
        unsigned int MailboxAddr = EIP130_MAILBOX_IN_BASE;
        MailboxAddr += (unsigned int)(EIP130_MAILBOX_SPACING_BYTES * (MailboxNr - 1));

        Device_Write32Array(Device,
                            MailboxAddr,
                            CommandToken_p->W,
                            EIP130TOKEN_COMMAND_WORDS);
    }

    // Handover the IN mailbox (containing the token) to the EIP130
    {
        uint32_t MailboxBit = BIT_0 << ((MailboxNr - 1) * 4);

        EIP130_RegisterWriteMailboxControl(Device, MailboxBit);
        if ((EIP130_RegisterReadMailboxStatus(Device) & MailboxBit) == 0)
        {
            // Check for possible race condition (Host slow compared to HW)
            // -> Assume token submit OK when the version register read
            //    provides the expected information
            uint32_t Value = EIP130_RegisterReadVersion(Device) & 0x0000FFFF;
            if ((Value != 0x7d82) && (Value != 0x738c))
            {
                return -4;              // Handover failed (Is HW active?)
            }
        }
    }

    return 0;                           // Success
}


/*----------------------------------------------------------------------------
 * EIP130_MailboxReadToken
 */
int
EIP130_MailboxReadToken(
        Device_Handle_t Device,
        const uint8_t MailboxNr,
        Eip130Token_Result_t * const ResultToken_p)
{
#ifdef EIP130_STRICT_ARGS
    if (MailboxNr < 1 || MailboxNr > 8)
    {
         return -1;
    }

    if (ResultToken_p == NULL)
    {
         return -2;
    }
#endif

    if (!EIP130_MailboxCanReadToken(Device, MailboxNr))
    {
        return -3;
    }

    // Copy the token from the OUT mailbox
    {
        unsigned int MailboxAddr = EIP130_MAILBOX_IN_BASE;
        MailboxAddr += (unsigned int)(EIP130_MAILBOX_SPACING_BYTES * (MailboxNr - 1));

        Device_Read32Array(Device,
                           MailboxAddr,
                           ResultToken_p->W,
                           EIP130TOKEN_RESULT_WORDS);
    }

	// Hand back the OUT mailbox to the EIP130
    {
        uint32_t MailboxBit = BIT_1 << ((MailboxNr - 1) * 4);

        EIP130_RegisterWriteMailboxControl(Device, MailboxBit);
    }

    return 0;                           // Success
}


/*----------------------------------------------------------------------------
 * EIP130_FirmwareCheck
 */
int
EIP130_FirmwareCheck(
        Device_Handle_t Device)
{
    uint32_t Value;
    int rc = 0;                         // Assume not written

    // Check for the hardware (EIP-130/140 or EIP-133)
    Value = EIP130_RegisterReadVersion(Device) & 0x0000FFFF;
    if ((Value != 0x7D82) && (Value != 0x7A85) && (Value != 0x738C))
    {
        return -1;                      // Not supported (Is HW active?)
    }

    // Check firmware configuration
    Value = EIP130_RegisterReadOptions2(Device);
    if ((Value & BIT_9) == 0)
    {
        return 2;                       // ROM firmware only
    }

    // RAM Firmware
    // - Check if firmware is written
    do
    {
        Value = EIP130_RegisterReadModuleStatus(Device);
    } while ((Value & EIP130_CRC24_BUSY) != 0);
    if (((Value & EIP130_CRC24_OK) == 0) ||
        ((Value & EIP130_FATAL_ERROR) != 0))
    {
        return -3;                      // Hardware error
    }
    if ((Value & EIP130_FIRMWARE_WRITTEN) == 0)
    {
        goto func_returnNotWritten;
    }

    // - Check if firmware checks are done & accepted
    if ((Value & EIP130_FIRMWARE_CHECKS_DONE) == 0)
    {
        rc = 1;                         // Still busy
    }
    else if ((Value & EIP130_FIRMWARE_ACCEPTED) != 0)
    {
        return 2;                       // Firmware is accepted
    }
    // else assume not written

func_returnNotWritten:
    // - Check Host ids - Is my_id equal to master_id?
    {
        uint8_t MyHostID;
        uint8_t MasterID;
#ifndef EIP130_ALLOW_MASTER_NONSECURE
        uint8_t MyProt_p;
        uint8_t ProtAvailable_p;
#endif

        EIP130_MailboxGetOptions(Device,
                                 &MyHostID, &MasterID,
#ifndef EIP130_ALLOW_MASTER_NONSECURE
                                 &MyProt_p, &ProtAvailable_p,
#else
                                 NULL, NULL,
#endif
                                 NULL, NULL);
        if ((MyHostID != MasterID)
#ifndef EIP130_ALLOW_MASTER_NONSECURE
            && (MyProt_p != ProtAvailable_p)
#endif
           )
        {
            // This Host is NOT allowed to do the firmware load
            return -2;
        }
    }
    return rc;                          // Firmware is NOT written/checks busy
}


#ifndef EIP130_REMOVE_FIRMWAREDOWNLOAD
/*----------------------------------------------------------------------------
 * EIP130_FirmwareLoad
 */
int
EIP130_FirmwareLoad(
        Device_Handle_t Device,
        const uint8_t MailboxNr,
        const uint32_t * const Firmware_p,
        const uint32_t FirmwareWord32Size)
{
    int rc;
    int nLoadRetries = 3;
    uint32_t Value = 0;

    // Check if firmware needs to be loaded
    rc = EIP130_FirmwareCheck(Device);
    if (rc < 0)
    {
        return rc;                      // General error
    }
    if (rc == 2)
    {
        return 0;                       // Firmware is already loaded and accepted (started)
    }
    if (rc == 1)
    {
        goto func_CheckAccepted;        // Firmware download is already busy
    }

    for (; nLoadRetries > 0; nLoadRetries--)
    {
        int nRetries;

        // Link mailbox
        // Note: The HW is expected to come out of reset state, so mailbox
        //       linking should be possible!
        rc = EIP130_MailboxLink(Device, MailboxNr);
        if (rc < 0)
        {
            return rc;                  // General error
        }

        // Check if mailbox is ready for the token
        // Note: The HW is expected to come out of reset state, so direct
        //       mailbox use should be possible!
        if (!EIP130_MailboxCanWriteToken(Device, MailboxNr))
        {
            (void)EIP130_MailboxUnlink(Device, MailboxNr); // Unlink mailbox
            return -1;                  // General error
        }

        // Write RAM-based Firmware Code Validation and Boot token (image header)
        // Note: The first 256 bytes of the firmware image
        rc = EIP130_MailboxWriteAndSubmitToken(Device, MailboxNr,
                                               (const Eip130Token_Command_t * const)Firmware_p);
        (void)EIP130_MailboxUnlink(Device, MailboxNr); // Unlink mailbox
        if (rc < 0)
        {
            return rc;                  // General error
        }

        // Write firmware code to FWRAM (image data)
        // Note: The firmware code is located directly behind the RAM-based
        //       Firmware Code Validation and Boot token in the firmware image.
        Device_Write32Array(Device, EIP130_FIRMWARE_RAM_BASE,
                            &Firmware_p[(256/4)], (int)(FirmwareWord32Size - (256/4)));

        // Report that the firmware is written
        EIP130_RegisterWriteModuleStatus(Device, EIP130_FIRMWARE_WRITTEN);
        Value = EIP130_RegisterReadModuleStatus(Device);
        if (((Value & EIP130_CRC24_OK) == 0) ||
            ((Value & EIP130_FATAL_ERROR) != 0))
        {
            return -5;                  // Hardware error
        }
        if ((Value & EIP130_FIRMWARE_WRITTEN) == 0)
        {
            return -1;                  // General error (Is HW active?)
        }

        // Check if firmware check is started
        for (nRetries = 0x0000FFFF; nRetries && ((Value & EIP130_FIRMWARE_CHECKS_DONE) != 0); nRetries--)
        {
            Value = EIP130_RegisterReadModuleStatus(Device);
        }
        if ((Value & EIP130_FIRMWARE_CHECKS_DONE) != 0)
        {
            return -3;                  // Timeout (Is HW active?)
        }

        // Check if firmware is accepted
func_CheckAccepted:
        for (nRetries = 0x7FFFFFFF; nRetries && ((Value & EIP130_FIRMWARE_CHECKS_DONE) == 0); nRetries--)
        {
            Value = EIP130_RegisterReadModuleStatus(Device);
        }
        if ((Value & EIP130_FIRMWARE_CHECKS_DONE) == 0)
        {
            return -3;                  // Timeout (Is HW active?)
        }
        if ((Value & EIP130_FIRMWARE_ACCEPTED) == EIP130_FIRMWARE_ACCEPTED)
        {
            return 0;                   // Firmware is accepted (started)
        }
    }
    return -4;                          // Firmware load failed
}
#endif


/* end of file eip130.c */
