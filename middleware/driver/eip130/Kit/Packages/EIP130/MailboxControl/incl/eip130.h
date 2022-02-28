/* eip130.h
 *
 * EIP-13x Root of Trust / Security Module Driver Library API
 */

/*****************************************************************************
* Copyright (c) 2014-2018 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef INCLUDE_GUARD_EIP130_H
#define INCLUDE_GUARD_EIP130_H

#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "device_types.h"           // Device_Handle_t
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t


/*----------------------------------------------------------------------------
 * EIP130_ModuleGetOptions
 *
 * This function can be used to retrieve specific module hardware related
 * information of this EIP-130. The pointer arguments define the information
 * that can be retrieved.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * fFWRam_p (optional)
 *     Pointer to the memory location where the Firmware RAM indication of this
 *     EIP-130 will be written. (0=No FWRAM, 1=FWRAM available)
 *
 * BusIfc_p (optional)
 *     Pointer to the memory location where the bus interface type of the
 *     EIP-130 will be written. (1=AXI, 0=AHB)
 *
 * Engines_p (optional)
 *     Pointer to the memory location where engine information of this EIP-130
 *     will be written.
 *
 * CustomEngines_p (optional)
 *     Pointer to the memory location where custom engine information of this
 *     EIP-130 will be written.
 */
void
EIP130_ModuleGetOptions(
        Device_Handle_t Device,
        uint8_t * const fFWRam_p,
        uint8_t * const BusIfc_p,
        uint8_t * const Engines_p,
        uint16_t * const CustomEngines_p);


/*----------------------------------------------------------------------------
 * EIP130_ModuleGetStatus
 *
 * This function can be used to retrieve module status information of this
 * EIP-130. The pointer arguments define the information that can be retrieved.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * fFIPSMode_p (optional)
 *     Pointer to the memory location where the FIPS mode indication of this
 *     EIP-130 will be written. (0=non-FIPS, 1=FIPS mode)
 *
 * fFatalError_p (optional)
 *     Pointer to the memory location where the fatal indication of this
 *     EIP-130 will be written. (0=no error, 1=fatal error)
 *
 * fCRC24Ok_p (optional)
 *     Pointer to the memory location where the CRC24 OK indication of this
 *     EIP-130 will be written. (0=busy/error, 1=OK)
 *
 * fCRC24Busy_p (optional)
 *     Pointer to the memory location where the CRC24 Busy indication of this
 *     EIP-130 will be written. (0=not busy, 1=Busy)
 *
 * fCRC24Error_p (optional)
 *     Pointer to the memory location where the CRC24 Error indication of this
 *     EIP-130 will be written. (0=no error, 1=error)
 *
 * fFirmwareWritten_p (optional)
 *     Pointer to the memory location where the firmware written indication of
 *     this EIP-130 will be written when FWRAM is available.
 *     (0=not written, 1=written)
 *
 * fFirmwareChecksDone_p (optional)
 *     Pointer to the memory location where the firmware checks done indication
 *     of this EIP-130 will be written when FWRAM is available.
 *     (0=not done/busy, 1=done)
 *
 * fFirmwareAccepted_p (optional)
 *     Pointer to the memory location where the firmware accepted indication of
 *     this EIP-130 will be written when FWRAM is available.
 *     (0=not accepted, 1=accepted)
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
        uint8_t * const fFirmwareAccepted_p);

/*----------------------------------------------------------------------------
 * EIP130_ModuleFirmwareWritten
 *
 * This function can be used to indicate that the firmware is written in FWRAM.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 */
void
EIP130_ModuleFirmwareWritten(
        Device_Handle_t Device);


/*----------------------------------------------------------------------------
 * EIP130_MailboxAccessVerify
 *
 * This function can be used to verify that the Device Handle really provides
 * communication to an EIP-130 hardware device.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * MailboxNr
 *     The mailbox number to write this token to (1..4).
 *     The mailbox must be linked to this host.
 *
 * Return Value:
 *     0    Success
 *     <0   Error code:
 *     -1   Not supported - Check if HW is active
 *     -2   Invalid mailbox number
 */
int
EIP130_MailboxAccessVerify(
        Device_Handle_t Device,
        const uint8_t MailboxNr);


/*----------------------------------------------------------------------------
 * EIP130_MailboxGetOptions
 *
 * This function can be used to retrieve specific mailbox related information
 * of this EIP-130. The pointer arguments define the information that can be
 * retrieved.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * MyHostID_p (optional)
 *     Pointer to the memory location where the HostID for this CPU will be
 *     written.
 *
 * MasterID_p (optional)
 *     Pointer to the memory location where the HostID of the Master CPU will
 *     be written. The Master CPU can use the EIP130_Mailbox_ControlAccess
 *     function.
 *
 * MyProt_p (optional)
 *     Pointer to the memory location where the protection setting for this CPU
 *     will be  written.
 *
 * ProtAvailable_p (optional)
 *     Pointer to the memory location where the protection available
 *     information will be written.
 *
 * NrOfMailboxes_p (optional)
 *     Pointer to the memory location where the number of mailboxes available
 *     on this EIP-130 will be written.
 *
 * MailboxSize_p (optional)
 *     Pointer to the memory location where the size of a mailbox of this
 *     EIP-130 will be written.
 */
void
EIP130_MailboxGetOptions(
        Device_Handle_t Device,
        uint8_t * const MyHostID_p,
        uint8_t * const MasterID_p,
        uint8_t * const MyProt_p,
        uint8_t * const ProtAvailable_p,
        uint8_t * const NrOfMailboxes_p,
        uint16_t * const MailboxSize_p);


/*----------------------------------------------------------------------------
 * EIP130_MailboxGetLinkID
 *
 * This function can be used to retrieve information about the host that
 * linked the mailbox.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * MailboxNr
 *     The mailbox number to retrieve the LinkID from (1..4).
 *
 * HostID_p
 *     Pointer to the memory location where the HostID will be written.
 *
 * Secure_p
 *     Pointer to the memory location where the indication will be written
 *     that the host is secure.
 *
 * Return Value:
 *     0    Success
 *     <0   Error code:
 *     -1   Invalid mailbox number
 *     -2   Not linked
 */
int
EIP130_MailboxGetLinkID(
        Device_Handle_t Device,
        const uint8_t MailboxNr,
        uint8_t * const HostID_p,
        uint8_t * const Secure_p);


/*----------------------------------------------------------------------------
 * EIP130_MailboxAccessControl
 *
 * This function is only available to the Master CPU and allows it to control
 * the maibox sharing by allowing only selected hosts to use selected
 * mailboxes. After reset, no host is allowed to access any mailbox.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * MailboxNr
 *     The number of the mailbox (1..M) for which to control the access.
 *
 * HostNr
 *     The number of the host (0..H) for which to control the access to
 *     MailboxNr.
 *
 * fAccessAllowed
 *     true   Allow HostNr to link and use MailboxNr.
 *     false  Do not allow HostNr to link and use MailboxNr.
 *
 * Return Value:
 *     0    Success
 *     <0   Error code:
 *     -1   Invalid mailbox number
 *     -2   Invalid host number
 */
int
EIP130_MailboxAccessControl(
        Device_Handle_t Device,
        uint8_t MailboxNr,
        uint8_t HostNr,
        bool fAccessAllowed);


/*----------------------------------------------------------------------------
 * EIP130_MailboxLink
 *
 * This function tries to link the requested mailbox. Upon success the mailbox
 * can be used until unlinked. A mailbox can only be linked by this host when
 * access has been granted by the Master CPU.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * MailboxNr
 *     The mailbox number to write this token to (1..4).
 *     The mailbox must be linked to this host.
 *
 * Return Value:
 *     0    Success
 *     <0   Error code:
 *     -1   Invalid mailbox number
 *     -2   Not link (linking failed)
 */
int
EIP130_MailboxLink(
        Device_Handle_t Device,
        const uint8_t MailboxNr);


/*----------------------------------------------------------------------------
 * EIP130_MailboxUnlink
 *
 * This function unlinks a previously linked mailbox.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * MailboxNr
 *     The mailbox number to write this token to (1..4).
 *     The mailbox must be linked to this host.
 *
 * Return Value:
 *     0    Success
 *     <0   Error code:
 *     -1   Invalid mailbox number
 *     -2   Still linked (unlinking failed)
 */
int
EIP130_MailboxUnlink(
        Device_Handle_t Device,
        const uint8_t MailboxNr);


/*----------------------------------------------------------------------------
 * EIP130_MailboxLinkReset
 *
 * This function resets a linked mailbox (forced unlinking).
 * Note: This function can only be called by the master host.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * MailboxNr
 *     The mailbox number to write this token to (1..4).
 *     The mailbox must be linked to this host.
 *
 * Return Value:
 *     0    Success
 *     <0   Error code:
 *     -1   Invalid mailbox number
 *     -2   Still linked (link reset failed)
 */
int
EIP130_MailboxLinkReset(
        Device_Handle_t Device,
        const uint8_t MailboxNr);


/*----------------------------------------------------------------------------
 * EIP130_MailboxCanReadToken/WriteToken
 *
 * CanWriteToken returns 'true' when a new command token may be written to the
 * mailbox of the EIP130 Security Module using one of the WriteToken functions.
 * CanReadToken returns 'true' when a result token can be read from the
 * mailbox of the EIP130 Security Module using one of the ReadToken functions.
 * This functions are fully re-entrant and thread-safe.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * MailboxNr
 *     The mailbox number to write this token to (1..4).
 *     The mailbox must be linked to this host.
 *
 * Return Value
 *     true   Token can be written/read
 *     false  Token canNOT be written/read
 */
bool
EIP130_MailboxCanWriteToken(
        Device_Handle_t Device,
        const uint8_t MailboxNr);

bool
EIP130_MailboxCanReadToken(
        Device_Handle_t Device,
        const uint8_t MailboxNr);


/*----------------------------------------------------------------------------
 * EIP130_MailboxWriteAndSubmitToken
 *
 * This function writes the token to the IN mailbox and then hands off the
 * mailbox to the SM to start processing the token. The request fails when the
 * mailbox is full or not linked to this host.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * MailboxNr
 *     The mailbox number to write this token to (1..4).
 *     The mailbox must be linked to this host.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Return Value
 *     0    Success
 *     <0   Error code:
 *     -1   Invalid mailbox number
 *     -2   Invalid CommandToken buffer specified
 *     -3   Token cannot be written
 *     -4   Token handover failed - Check if HW is active
 */
int
EIP130_MailboxWriteAndSubmitToken(
        Device_Handle_t Device,
        const uint8_t MailboxNr,
        const Eip130Token_Command_t * const CommandToken_p);


/*----------------------------------------------------------------------------
 * EIP130_MailboxReadToken
 *
 * This function reads the token from the OUT mailbox and then frees the
 * mailbox, allowing the SM to write another result in it.
 * The request fails when the mailbox is empty or the token is not for this
 * host.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * MailboxNr
 *     The mailbox number to write this token to (1..4).
 *     The mailbox must be linked to this host.
 *
 * ResultToken_p
 *     Pointer to the result token buffer this function will write to.
 *
 * Return Value
 *     0    Success
 *     <0   Error code:
 *     -1   Invalid mailbox number
 *     -2   Invalid ResultToken buffer specified
 *     -3   Token cannot be read
 */
int
EIP130_MailboxReadToken(
        Device_Handle_t Device,
        const uint8_t MailboxNr,
        Eip130Token_Result_t * const ResultToken_p);


/*----------------------------------------------------------------------------
 * EIP130_FirmwareCheck
 *
 * This function checks if firmware needs to be loaded.
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * Return Value
 *     2    Firmware is ready to use
 *     1    Firmware checks are still busy
 *     0    Firmware needs to be loaded
 *     <0   Error code:
 *     -1   Not supported - Check if HW is active
 *     -2   This Host is NOT allowed to do the download
 *     -3   Hardware error
 */
int
EIP130_FirmwareCheck(
        Device_Handle_t Device);


/*----------------------------------------------------------------------------
 * EIP130_FirmwareLoad
 *
 * This function first checks if the firmware needs to be loaded. If that is
 * the case it writes the appropriate firmware image parts to mailbox 1 and
 * FWRAM and waits till the firmware has been accepted (and thus started).
 *
 * Device
 *     The Driver Framework Device Handle for the EIP-130.
 *
 * MailboxNr
 *     The mailbox number to use for the RAM-based Firmware Code Validation
 *     and Boot token. Note mailbox will be unlink after function return.
 *
 * Firmware_p
 *     Pointer to the buffer that holds the firmware image.
 *
 * FirmwareWord32Size
 *     The size of the firmware image in 32-bit words.
 *
 * Return Value
 *     0    Firmware load and accepted (started)
 *     <0   Error code:
 *     -1   Not supported - Check if HW is active
 *     -2   This Host is NOT allowed to do the download
 *     -3   Firmware checks done timeout - Check if HW is active
 *     -4   Firmware load failed (multiple times)
 *     -5   Hardware error
 */
int
EIP130_FirmwareLoad(
        Device_Handle_t Device,
        const uint8_t MailboxNr,
        const uint32_t * const Firmware_p,
        const uint32_t FirmwareWord32Size);


#endif /* Include Guard */


/* end of file eip130.h */
