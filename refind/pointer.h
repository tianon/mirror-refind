/*
 * refind/pointer.h
 * pointer device functions header file
 */

#ifndef __REFIND_POINTERDEVICE_H_
#define __REFIND_POINTERDEVICE_H_

#ifdef __MAKEWITH_GNUEFI
#include "efi.h"
#include "efilib.h"
#else
#include "../include/tiano_includes.h"
#endif

#ifndef _EFI_POINT_H
#include "../EfiLib/AbsolutePointer.h"
#endif

typedef struct PointerStateStruct {
    UINTN X, Y;
    BOOLEAN Press;
    BOOLEAN Holding;
} POINTER_STATE;

VOID pdInitialize();
VOID pdCleanup();
BOOLEAN pdAvailable();
UINTN pdCount();
EFI_EVENT pdWaitEvent(IN UINTN Index);
EFI_STATUS pdUpdateState();
POINTER_STATE pdGetState();

VOID pdDraw();
VOID pdClear();

#endif

/* EOF */
