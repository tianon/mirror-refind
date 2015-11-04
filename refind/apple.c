/*
 * refind/apple.c
 * Functions specific to Apple computers
 * 
 * Copyright (c) 2015 Roderick W. Smith
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include "global.h"
#include "config.h"
#include "lib.h"
#include "screen.h"
#include "apple.h"
#include "refit_call_wrapper.h"

CHAR16 *gCsrStatus = NULL;

// Get CSR (Apple's System Integrity Protection [SIP], or "rootless") status
// byte. Return values:
// -2      = Call succeeded but returned value of unexpected length, so result
//           is suspect
// -1      = Call failed; likely not an Apple, or an Apple running OS X version
//           that doesn't support CSR/SIP
// 0-127   = Valid values (as of 11/2015)
// 128-255 = High bit set unexpectedly, but value still returned
INTN GetCsrStatus(VOID) {
    CHAR8 *CsrValues;
    UINTN CsrLength;
    EFI_GUID CsrGuid = CSR_GUID;
    EFI_STATUS Status;

    Status = EfivarGetRaw(&CsrGuid, L"csr-active-config", &CsrValues, &CsrLength);
    if (Status == EFI_SUCCESS) {
        if (CsrLength == 4) {
            return CsrValues[0];
        } else {
            return -2;
        }
    } else {
        return -1;
    }
} // INTN GetCsrStatus()

// Store string describing CSR status byte in gCsrStatus variable, which appears
// on the Info page.
VOID RecordgCsrStatus(INTN CsrStatus) {
    if (gCsrStatus == NULL)
        gCsrStatus = AllocateZeroPool(256 * sizeof(CHAR16));

    switch (CsrStatus) {
        case -2:
            SPrint(gCsrStatus, 255, L" System Integrity Protection status is unrecognized");
            break;
        case -1:
            SPrint(gCsrStatus, 255, L"System Integrity Protection status is unrecorded");
            break;
        case SIP_ENABLED:
            SPrint(gCsrStatus, 255, L" System Integrity Protection is enabled (0x%02x)", CsrStatus);
            break;
        case SIP_DISABLED:
            SPrint(gCsrStatus, 255, L" System Integrity Protection is disabled (0x%02x)", CsrStatus);
            break;
        default:
            SPrint(gCsrStatus, 255, L" System Integrity Protection status: 0x%02x", CsrStatus);
    } // switch
} // VOID RecordgCsrStatus

// Find the current CSR status and reset it to the next one in the
// GlobalConfig.CsrValues list, or to the first value if the current
// value is not on the list.
// Returns the value to which the CSR is being set.
INTN RotateCsrValue(VOID) {
    INTN        CurrentValue;
    UINTN       Index = 0;
    CHAR16      *CurrentValueAsString = NULL;
    CHAR16      *TargetValueAsString = NULL;
    CHAR16      *ListItem;
    CHAR8       TargetCsr[4];
    EFI_GUID    CsrGuid = CSR_GUID;
    EFI_STATUS  Status;
    EG_PIXEL    BGColor;

    BGColor.b = 255;
    BGColor.g = 175;
    BGColor.r = 100;
    BGColor.a = 0;
    CurrentValue = GetCsrStatus();
    if ((CurrentValue >= 0) && GlobalConfig.CsrValues) {
        CurrentValueAsString = PoolPrint(L"%02x", CurrentValue);
        while (TargetValueAsString == NULL) {
            ListItem = FindCommaDelimited(GlobalConfig.CsrValues, Index++);
            if (ListItem) {
                if (MyStriCmp(ListItem, CurrentValueAsString)) {
                    TargetValueAsString = FindCommaDelimited(GlobalConfig.CsrValues, Index);
                    if (TargetValueAsString == NULL)
                        TargetValueAsString = FindCommaDelimited(GlobalConfig.CsrValues, 0);
                }
            } else {
                TargetValueAsString = FindCommaDelimited(GlobalConfig.CsrValues, 0);
            } // if/else
            MyFreePool(ListItem);
        } // while
        TargetCsr[0] = (CHAR8) StrToHex(TargetValueAsString, 0, 2);
        Status = EfivarSetRaw(&CsrGuid, L"csr-active-config", TargetCsr, 4, TRUE);
        if (Status == EFI_SUCCESS) {
            switch (TargetCsr[0]) {
                case SIP_ENABLED:
                    egDisplayMessage(PoolPrint(L"Set System Integrity Protection to enabled (0x%x)", (UINTN) TargetCsr[0]), &BGColor);
                    break;
                case SIP_DISABLED:
                    egDisplayMessage(PoolPrint(L"Set System Integrity Protection status to disabled (0x%x)", (UINTN) TargetCsr[0]), &BGColor);
                    break;
                default:
                    egDisplayMessage(PoolPrint(L"Set System Integrity Protection status to 0x%x", (UINTN) TargetCsr[0]), &BGColor);
            }
            RecordgCsrStatus((INTN) TargetCsr[0]);
        } else {
            egDisplayMessage(L"Error setting System Integrity Protection status", &BGColor);
        }
        PauseSeconds(3);
    } // if
    return (INTN) TargetCsr[0];
} // INTN RotateCsrValue()


/*
 * The below definitions and SetAppleOSInfo() function are based on a GRUB patch
 * by Andreas Heider:
 * https://lists.gnu.org/archive/html/grub-devel/2013-12/msg00442.html
 */

#define EFI_APPLE_SET_OS_PROTOCOL_GUID  \
  { 0xc5c5da95, 0x7d5c, 0x45e6, \
    { 0xb2, 0xf1, 0x3f, 0xd5, 0x2b, 0xb1, 0x00, 0x77 } \
  }

typedef struct EfiAppleSetOsInterface {
    UINT64 Version;
    EFI_STATUS EFIAPI (*SetOsVersion) (IN CHAR8 *Version);
    EFI_STATUS EFIAPI (*SetOsVendor) (IN CHAR8 *Vendor);
} EfiAppleSetOsInterface;

// Function to tell the firmware that OS X is being launched. This is
// required to work around problems on some Macs that don't fully
// initialize some hardware (especially video displays) when third-party
// OSes are launched in EFI mode.
EFI_STATUS SetAppleOSInfo() {
    CHAR16 *AppleOSVersion = NULL;
    CHAR8 *AppleOSVersion8 = NULL;
    EFI_STATUS Status;
    EFI_GUID apple_set_os_guid = EFI_APPLE_SET_OS_PROTOCOL_GUID;
    EfiAppleSetOsInterface *SetOs = NULL;

    Status = refit_call3_wrapper(BS->LocateProtocol, &apple_set_os_guid, NULL, (VOID**) &SetOs);

    // Not a Mac, so ignore the call....
    if ((Status != EFI_SUCCESS) || (!SetOs))
        return EFI_SUCCESS;

    if ((SetOs->Version != 0) && GlobalConfig.SpoofOSXVersion) {
        AppleOSVersion = StrDuplicate(L"Mac OS X");
        MergeStrings(&AppleOSVersion, GlobalConfig.SpoofOSXVersion, ' ');
        if (AppleOSVersion) {
            AppleOSVersion8 = AllocateZeroPool((StrLen(AppleOSVersion) + 1) * sizeof(CHAR8));
            UnicodeStrToAsciiStr(AppleOSVersion, AppleOSVersion8);
            if (AppleOSVersion8) {
                Status = refit_call1_wrapper (SetOs->SetOsVersion, AppleOSVersion8);
                if (!EFI_ERROR(Status))
                    Status = EFI_SUCCESS;
                MyFreePool(AppleOSVersion8);
            } else {
                Status = EFI_OUT_OF_RESOURCES;
                Print(L"Out of resources in SetAppleOSInfo!\n");
            }
            if ((Status == EFI_SUCCESS) && (SetOs->Version == 2))
                Status = refit_call1_wrapper (SetOs->SetOsVendor, "Apple Inc.");
            MyFreePool(AppleOSVersion);
        } // if (AppleOSVersion)
    } // if
    if (Status != EFI_SUCCESS)
        Print(L"Unable to set firmware boot type!\n");

    return (Status);
} // EFI_STATUS SetAppleOSInfo()