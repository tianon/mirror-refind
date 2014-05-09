/*
 * EfiLib/gnuefi.h
 * Header file for GNU-EFI support in legacy boot code
 *
 * Copyright (c) 2014 Roderick W. Smith
 * With extensive borrowing from other sources (mostly Tianocore)
 *
 * This software is licensed under the terms of the GNU GPLv3,
 * a copy of which should come with this file.
 *
 */
/*
 * THIS FILE SHOULD NOT BE INCLUDED WHEN COMPILING UNDER TIANOCORE'S TOOLKIT!
 */

#ifndef __EFILIB_GNUEFI_H
#define __EFILIB_GNUEFI_H

#include "efi.h"
#include "efilib.h"

#define EFI_DEVICE_PATH_PROTOCOL EFI_DEVICE_PATH
#define UnicodeSPrint SPrint
#define gRT RT
#define gBS BS
#define CONST
#define ASSERT_EFI_ERROR(status)  ASSERT(!EFI_ERROR(status))

CHAR8 *
UnicodeStrToAsciiStr (
   IN       CHAR16              *Source,
   OUT      CHAR8               *Destination
);

UINTN
AsciiStrLen (
   IN      CONST CHAR8               *String
);

UINTN
EFIAPI
GetDevicePathSize (
   IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
);

EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
GetNextDevicePathInstance (
   IN OUT EFI_DEVICE_PATH_PROTOCOL    **DevicePath,
   OUT UINTN                          *Size
);

#endif