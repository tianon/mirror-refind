/*
 * EfiLib/legacy.c
 * CSM/legacy boot support functions
 * 
 * Taken from Tianocore source code (mostly IntelFrameworkModulePkg/Universal/BdsDxe/BootMaint/BBSsupport.c)
 *
 * Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
 * This program and the accompanying materials
 * are licensed and made available under the terms and conditions of the BSD License
 * which accompanies this distribution.  The full text of the license may be found at
 * http://opensource.org/licenses/bsd-license.php
 * 
 * THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 * WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 *
 */

#include "../include/tiano_includes.h"
#include "legacy.h"

BOOT_OPTION_BBS_MAPPING  *mBootOptionBbsMapping     = NULL;
UINTN                    mBootOptionBbsMappingCount = 0;

extern EFI_DEVICE_PATH EndDevicePath[];
extern EFI_GUID gEfiLegacyBiosProtocolGuid;
EFI_GUID gEfiLegacyDevOrderVariableGuid     = { 0xa56074db, 0x65fe, 0x45f7, {0xbd, 0x21, 0x2d, 0x2b, 0xdd, 0x8e, 0x96, 0x52 }};

/**

  Translate the first n characters of an Ascii string to
  Unicode characters. The count n is indicated by parameter
  Size. If Size is greater than the length of string, then
  the entire string is translated.


  @param AStr               Pointer to input Ascii string.
  @param Size               The number of characters to translate.
  @param UStr               Pointer to output Unicode string buffer.

**/
VOID
AsciiToUnicodeSize (
  IN UINT8              *AStr,
  IN UINTN              Size,
  OUT UINT16            *UStr
  )
{
  UINTN Idx;

  Idx = 0;
  while (AStr[Idx] != 0) {
    UStr[Idx] = (CHAR16) AStr[Idx];
    if (Idx == Size) {
      break;
    }

    Idx++;
  }
  UStr[Idx] = 0;
}

/**
  Build Legacy Device Name String according.

  @param CurBBSEntry     BBS Table.
  @param Index           Index.
  @param BufSize         The buffer size.
  @param BootString      The output string.

**/
VOID
BdsBuildLegacyDevNameString (
  IN  BBS_TABLE                 *CurBBSEntry,
  IN  UINTN                     Index,
  IN  UINTN                     BufSize,
  OUT CHAR16                    *BootString
  )
{
  CHAR16  *Fmt;
  CHAR16  *Type;
  UINT8   *StringDesc;
  CHAR16  Temp[80];

  switch (Index) {
  //
  // Primary Master
  //
  case 1:
    Fmt = L"Primary Master %s";
    break;

 //
 // Primary Slave
 //
  case 2:
    Fmt = L"Primary Slave %s";
    break;

  //
  // Secondary Master
  //
  case 3:
    Fmt = L"Secondary Master %s";
    break;

  //
  // Secondary Slave
  //
  case 4:
    Fmt = L"Secondary Slave %s";
    break;

  default:
    Fmt = L"%s";
    break;
  }

  switch (CurBBSEntry->DeviceType) {
  case BBS_FLOPPY:
    Type = L"Floppy";
    break;

  case BBS_HARDDISK:
    Type = L"Harddisk";
    break;

  case BBS_CDROM:
    Type = L"CDROM";
    break;

  case BBS_PCMCIA:
    Type = L"PCMCIAe";
    break;

  case BBS_USB:
    Type = L"USB";
    break;

  case BBS_EMBED_NETWORK:
    Type = L"Network";
    break;

  case BBS_BEV_DEVICE:
    Type = L"BEVe";
    break;

  case BBS_UNKNOWN:
  default:
    Type = L"Unknown";
    break;
  }
  //
  // If current BBS entry has its description then use it.
  //
  StringDesc = (UINT8 *) (UINTN) ((CurBBSEntry->DescStringSegment << 4) + CurBBSEntry->DescStringOffset);
  if (NULL != StringDesc) {
    //
    // Only get fisrt 32 characters, this is suggested by BBS spec
    //
    AsciiToUnicodeSize (StringDesc, 32, Temp);
    Fmt   = L"%s";
    Type  = Temp;
  }

  //
  // BbsTable 16 entries are for onboard IDE.
  // Set description string for SATA harddisks, Harddisk 0 ~ Harddisk 11
  //
  if (Index >= 5 && Index <= 16 && (CurBBSEntry->DeviceType == BBS_HARDDISK || CurBBSEntry->DeviceType == BBS_CDROM)) {
    Fmt = L"%s %d";
    UnicodeSPrint (BootString, BufSize, Fmt, Type, Index - 5);
  } else {
    UnicodeSPrint (BootString, BufSize, Fmt, Type);
  }
}

/**
  Check if the boot option is a legacy one.

  @param BootOptionVar   The boot option data payload.
  @param BbsEntry        The BBS Table.
  @param BbsIndex        The table index.

  @retval TRUE           It is a legacy boot option.
  @retval FALSE          It is not a legacy boot option.

**/
BOOLEAN
BdsIsLegacyBootOption (
  IN UINT8                 *BootOptionVar,
  OUT BBS_TABLE            **BbsEntry,
  OUT UINT16               *BbsIndex
  )
{
  UINT8                     *Ptr;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  BOOLEAN                   Ret;
  UINT16                    DevPathLen;

  Ptr = BootOptionVar;
  Ptr += sizeof (UINT32);
  DevPathLen = *(UINT16 *) Ptr;
  Ptr += sizeof (UINT16);
  Ptr += StrSize ((UINT16 *) Ptr);
  DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) Ptr;
  if ((BBS_DEVICE_PATH == DevicePath->Type) && (BBS_BBS_DP == DevicePath->SubType)) {
    Ptr += DevPathLen;
    *BbsEntry = (BBS_TABLE *) Ptr;
    Ptr += sizeof (BBS_TABLE);
    *BbsIndex = *(UINT16 *) Ptr;
    Ret       = TRUE;
  } else {
    *BbsEntry = NULL;
    Ret       = FALSE;
  }

  return Ret;
}

/**
  Find all legacy boot option by device type.

  @param BootOrder       The boot order array.
  @param BootOptionNum   The number of boot option.
  @param DevType         Device type.
  @param DevName         Device name.
  @param Attribute       The boot option attribute.
  @param BbsIndex        The BBS table index.
  @param OptionNumber    The boot option index.

  @retval TRUE           The Legacy boot option is found.
  @retval FALSE          The legacy boot option is not found.

**/
BOOLEAN
BdsFindLegacyBootOptionByDevTypeAndName (
  IN UINT16                 *BootOrder,
  IN UINTN                  BootOptionNum,
  IN UINT16                 DevType,
  IN CHAR16                 *DevName,
  OUT UINT32                *Attribute,
  OUT UINT16                *BbsIndex,
  OUT UINT16                *OptionNumber
  )
{
  UINTN     Index;
  CHAR16    BootOption[9];
  UINTN     BootOptionSize;
  UINT8     *BootOptionVar;
  BBS_TABLE *BbsEntry;
  BOOLEAN   Found;

  BbsEntry  = NULL;
  Found     = FALSE;

  if (NULL == BootOrder) {
    return Found;
  }

  //
  // Loop all boot option from variable
  //
  for (Index = 0; Index < BootOptionNum; Index++) {
    UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", (UINTN) BootOrder[Index]);
    BootOptionVar = BdsLibGetVariableAndSize (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      &BootOptionSize
                      );
    if (NULL == BootOptionVar) {
      continue;
    }

    //
    // Skip Non-legacy boot option
    //
    if (!BdsIsLegacyBootOption (BootOptionVar, &BbsEntry, BbsIndex)) {
      FreePool (BootOptionVar);
      continue;
    }

    if (
        (BbsEntry->DeviceType != DevType) ||
        (StrCmp (DevName, (CHAR16*)(BootOptionVar + sizeof (UINT32) + sizeof (UINT16))) != 0)
       ) {
      FreePool (BootOptionVar);
      continue;
    }

    *Attribute    = *(UINT32 *) BootOptionVar;
    *OptionNumber = BootOrder[Index];
    Found         = TRUE;
    FreePool (BootOptionVar);
    break;
  }

  return Found;
}

/**

  Create a legacy boot option for the specified entry of
  BBS table, save it as variable, and append it to the boot
  order list.


  @param CurrentBbsEntry    Pointer to current BBS table.
  @param CurrentBbsDevPath  Pointer to the Device Path Protocol instance of BBS
  @param Index              Index of the specified entry in BBS table.
  @param BootOrderList      On input, the original boot order list.
                            On output, the new boot order list attached with the
                            created node.
  @param BootOrderListSize  On input, the original size of boot order list.
                            On output, the size of new boot order list.

  @retval  EFI_SUCCESS             Boot Option successfully created.
  @retval  EFI_OUT_OF_RESOURCES    Fail to allocate necessary memory.
  @retval  Other                   Error occurs while setting variable.

**/
EFI_STATUS
BdsCreateLegacyBootOption (
  IN BBS_TABLE                        *CurrentBbsEntry,
  IN EFI_DEVICE_PATH_PROTOCOL         *CurrentBbsDevPath,
  IN UINTN                            Index,
  IN OUT UINT16                       **BootOrderList,
  IN OUT UINTN                        *BootOrderListSize
  )
{
  EFI_STATUS           Status;
  UINT16               CurrentBootOptionNo;
  UINT16               BootString[10];
  CHAR16               BootDesc[100];
  CHAR8                HelpString[100];
  UINT16               *NewBootOrderList;
  UINTN                BufferSize;
  UINTN                StringLen;
  VOID                 *Buffer;
  UINT8                *Ptr;
  UINT16               CurrentBbsDevPathSize;
  UINTN                BootOrderIndex;
  UINTN                BootOrderLastIndex;
  UINTN                ArrayIndex;
  BOOLEAN              IndexNotFound;
  BBS_BBS_DEVICE_PATH  *NewBbsDevPathNode;

  if ((*BootOrderList) == NULL) {
    CurrentBootOptionNo = 0;
  } else {
    for (ArrayIndex = 0; ArrayIndex < (UINTN) (*BootOrderListSize / sizeof (UINT16)); ArrayIndex++) {
      IndexNotFound = TRUE;
      for (BootOrderIndex = 0; BootOrderIndex < (UINTN) (*BootOrderListSize / sizeof (UINT16)); BootOrderIndex++) {
        if ((*BootOrderList)[BootOrderIndex] == ArrayIndex) {
          IndexNotFound = FALSE;
          break;
        }
      }

      if (!IndexNotFound) {
        continue;
      } else {
        break;
      }
    }

    CurrentBootOptionNo = (UINT16) ArrayIndex;
  }

  UnicodeSPrint (
    BootString,
    sizeof (BootString),
    L"Boot%04x",
    CurrentBootOptionNo
    );

  BdsBuildLegacyDevNameString (CurrentBbsEntry, Index, sizeof (BootDesc), BootDesc);

  //
  // Create new BBS device path node with description string
  //
  UnicodeStrToAsciiStr (BootDesc, HelpString);

  StringLen = AsciiStrLen (HelpString);
  NewBbsDevPathNode = AllocateZeroPool (sizeof (BBS_BBS_DEVICE_PATH) + StringLen);
  if (NewBbsDevPathNode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (NewBbsDevPathNode, CurrentBbsDevPath, sizeof (BBS_BBS_DEVICE_PATH));
  CopyMem (NewBbsDevPathNode->String, HelpString, StringLen + 1);
  SetDevicePathNodeLength (&(NewBbsDevPathNode->Header), sizeof (BBS_BBS_DEVICE_PATH) + StringLen);

  //
  // Create entire new CurrentBbsDevPath with end node
  //
  CurrentBbsDevPath = AppendDevicePathNode (
                        EndDevicePath,
                        (EFI_DEVICE_PATH_PROTOCOL *) NewBbsDevPathNode
                        );
   if (CurrentBbsDevPath == NULL) {
    FreePool (NewBbsDevPathNode);
    return EFI_OUT_OF_RESOURCES;
  }

  CurrentBbsDevPathSize = (UINT16) (GetDevicePathSize (CurrentBbsDevPath));

  BufferSize = sizeof (UINT32) +
    sizeof (UINT16) +
    StrSize (BootDesc) +
    CurrentBbsDevPathSize +
    sizeof (BBS_TABLE) +
    sizeof (UINT16);

  Buffer = AllocateZeroPool (BufferSize);
  if (Buffer == NULL) {
    FreePool (NewBbsDevPathNode);
    FreePool (CurrentBbsDevPath);
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr               = (UINT8 *) Buffer;

  *((UINT32 *) Ptr) = LOAD_OPTION_ACTIVE;
  Ptr += sizeof (UINT32);

  *((UINT16 *) Ptr) = CurrentBbsDevPathSize;
  Ptr += sizeof (UINT16);

  CopyMem (
    Ptr,
    BootDesc,
    StrSize (BootDesc)
    );
  Ptr += StrSize (BootDesc);

  CopyMem (
    Ptr,
    CurrentBbsDevPath,
    CurrentBbsDevPathSize
    );
  Ptr += CurrentBbsDevPathSize;

  CopyMem (
    Ptr,
    CurrentBbsEntry,
    sizeof (BBS_TABLE)
    );

  Ptr += sizeof (BBS_TABLE);
  *((UINT16 *) Ptr) = (UINT16) Index;

  Status = gRT->SetVariable (
                  BootString,
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  BufferSize,
                  Buffer
                  );

  FreePool (Buffer);
  
  Buffer = NULL;

  NewBootOrderList = AllocateZeroPool (*BootOrderListSize + sizeof (UINT16));
  if (NULL == NewBootOrderList) {
    FreePool (NewBbsDevPathNode);
    FreePool (CurrentBbsDevPath);
    return EFI_OUT_OF_RESOURCES;
  }

  if (*BootOrderList != NULL) {
    CopyMem (NewBootOrderList, *BootOrderList, *BootOrderListSize);
    FreePool (*BootOrderList);
  }

  BootOrderLastIndex                    = (UINTN) (*BootOrderListSize / sizeof (UINT16));
  NewBootOrderList[BootOrderLastIndex]  = CurrentBootOptionNo;
  *BootOrderListSize += sizeof (UINT16);
  *BootOrderList = NewBootOrderList;

  FreePool (NewBbsDevPathNode);
  FreePool (CurrentBbsDevPath);
  return Status;
}

/**
  Create a legacy boot option.

  @param BbsItem         The BBS Table entry.
  @param Index           Index of the specified entry in BBS table.
  @param BootOrderList   The boot order list.
  @param BootOrderListSize The size of boot order list.

  @retval EFI_OUT_OF_RESOURCE  No enough memory.
  @retval EFI_SUCCESS          The function complete successfully.
  @return Other value if the legacy boot option is not created.

**/
EFI_STATUS
BdsCreateOneLegacyBootOption (
  IN BBS_TABLE              *BbsItem,
  IN UINTN                  Index,
  IN OUT UINT16             **BootOrderList,
  IN OUT UINTN              *BootOrderListSize
  )
{
  BBS_BBS_DEVICE_PATH       BbsDevPathNode;
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;

  DevPath                       = NULL;

  //
  // Create device path node.
  //
  BbsDevPathNode.Header.Type    = BBS_DEVICE_PATH;
  BbsDevPathNode.Header.SubType = BBS_BBS_DP;
  SetDevicePathNodeLength (&BbsDevPathNode.Header, sizeof (BBS_BBS_DEVICE_PATH));
  BbsDevPathNode.DeviceType = BbsItem->DeviceType;
  CopyMem (&BbsDevPathNode.StatusFlag, &BbsItem->StatusFlags, sizeof (UINT16));

  DevPath = AppendDevicePathNode (
              EndDevicePath,
              (EFI_DEVICE_PATH_PROTOCOL *) &BbsDevPathNode
              );
  if (NULL == DevPath) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = BdsCreateLegacyBootOption (
            BbsItem,
            DevPath,
            Index,
            BootOrderList,
            BootOrderListSize
            );
  BbsItem->BootPriority = 0x00;

  FreePool (DevPath);

  return Status;
}

/**
  Group the legacy boot options in the BootOption.

  The routine assumes the boot options in the beginning that covers all the device 
  types are ordered properly and re-position the following boot options just after
  the corresponding boot options with the same device type.
  For example:
  1. Input  = [Harddisk1 CdRom2 Efi1 Harddisk0 CdRom0 CdRom1 Harddisk2 Efi0]
     Assuming [Harddisk1 CdRom2 Efi1] is ordered properly
     Output = [Harddisk1 Harddisk0 Harddisk2 CdRom2 CdRom0 CdRom1 Efi1 Efi0]

  2. Input  = [Efi1 Efi0 CdRom1 Harddisk0 Harddisk1 Harddisk2 CdRom0 CdRom2]
     Assuming [Efi1 Efi0 CdRom1 Harddisk0] is ordered properly
     Output = [Efi1 Efi0 CdRom1 CdRom0 CdRom2 Harddisk0 Harddisk1 Harddisk2]

  @param BootOption      Pointer to buffer containing Boot Option Numbers
  @param BootOptionCount Count of the Boot Option Numbers
**/
VOID
GroupMultipleLegacyBootOption4SameType (
  UINT16                   *BootOption,
  UINTN                    BootOptionCount
  )
{
  UINTN                    DeviceTypeIndex[7];
  UINTN                    Index;
  UINTN                    MappingIndex;
  UINTN                    *NextIndex;
  UINT16                   OptionNumber;
  UINTN                    DeviceIndex;

  SetMem (DeviceTypeIndex, sizeof (DeviceTypeIndex), 0xFF);

  for (Index = 0; Index < BootOptionCount; Index++) {

    //
    // Find the DeviceType
    //
    for (MappingIndex = 0; MappingIndex < mBootOptionBbsMappingCount; MappingIndex++) {
      if (mBootOptionBbsMapping[MappingIndex].BootOptionNumber == BootOption[Index]) {
        break;
      }
    }
    if (MappingIndex == mBootOptionBbsMappingCount) {
      //
      // Is not a legacy boot option
      //
      continue;
    }

    ASSERT ((mBootOptionBbsMapping[MappingIndex].BbsType & 0xF) < 
             sizeof (DeviceTypeIndex) / sizeof (DeviceTypeIndex[0]));
    NextIndex = &DeviceTypeIndex[mBootOptionBbsMapping[MappingIndex].BbsType & 0xF];
    if (*NextIndex == (UINTN) -1) {
      //
      // *NextIndex is the index in BootOption to put the next Option Number for the same type
      //
      *NextIndex = Index + 1;
    } else {
      //
      // insert the current boot option before *NextIndex, causing [*Next .. Index] shift right one position
      //
      OptionNumber = BootOption[Index];
      CopyMem (&BootOption[*NextIndex + 1], &BootOption[*NextIndex], (Index - *NextIndex) * sizeof (UINT16));
      BootOption[*NextIndex] = OptionNumber;

      //
      // Update the DeviceTypeIndex array to reflect the right shift operation
      //
      for (DeviceIndex = 0; DeviceIndex < sizeof (DeviceTypeIndex) / sizeof (DeviceTypeIndex[0]); DeviceIndex++) {
        if (DeviceTypeIndex[DeviceIndex] != (UINTN) -1 && DeviceTypeIndex[DeviceIndex] >= *NextIndex) {
          DeviceTypeIndex[DeviceIndex]++;
        }
      }
    }
  }
}

/**
  Function returns the value of the specified variable.


  @param Name            A Null-terminated Unicode string that is
                         the name of the vendor's variable.
  @param VendorGuid      A unique identifier for the vendor.

  @return               The payload of the variable.
  @retval NULL          If the variable can't be read.

**/
VOID *
EfiLibGetVariable (
  IN CHAR16               *Name,
  IN EFI_GUID             *VendorGuid
  )
{
  UINTN VarSize;

  return BdsLibGetVariableAndSize (Name, VendorGuid, &VarSize);
}

/**
  Function deletes the variable specified by VarName and VarGuid.

  @param VarName           A Null-terminated Unicode string that is
                           the name of the vendor's variable.
                         
  @param VarGuid           A unique identifier for the vendor.

  @retval  EFI_SUCCESS           The variable was found and removed
  @retval  EFI_UNSUPPORTED       The variable store was inaccessible
  @retval  EFI_OUT_OF_RESOURCES  The temporary buffer was not available
  @retval  EFI_NOT_FOUND         The variable was not found

**/
EFI_STATUS
EfiLibDeleteVariable (
  IN CHAR16   *VarName,
  IN EFI_GUID *VarGuid
  )
{
  VOID        *VarBuf;
  EFI_STATUS  Status;

  VarBuf  = EfiLibGetVariable (VarName, VarGuid);
  Status  = EFI_NOT_FOUND;

  if (VarBuf != NULL) {
    //
    // Delete variable from Storage
    //
    Status = gRT->SetVariable (VarName, VarGuid, VAR_FLAG, 0, NULL);
    ASSERT (!EFI_ERROR (Status));
    FreePool (VarBuf);
  }

  return Status;
}

/**
  Add the legacy boot options from BBS table if they do not exist.

  @retval EFI_SUCCESS          The boot options are added successfully 
                               or they are already in boot options.
  @retval EFI_NOT_FOUND        No legacy boot options is found.
  @retval EFI_OUT_OF_RESOURCE  No enough memory.
  @return Other value          LegacyBoot options are not added.
**/
EFI_STATUS
EFIAPI
BdsAddNonExistingLegacyBootOptions (
  VOID
  )
{
  UINT16                    *BootOrder;
  UINTN                     BootOrderSize;
  EFI_STATUS                Status;
  CHAR16                    Desc[100];
  UINT16                    HddCount;
  UINT16                    BbsCount;
  HDD_INFO                  *LocalHddInfo;
  BBS_TABLE                 *LocalBbsTable;
  UINT16                    BbsIndex;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  UINT16                    Index;
  UINT32                    Attribute;
  UINT16                    OptionNumber;
  BOOLEAN                   Exist;

  Print(L"Entering BdsAddNonExistingLegacyBootOptions()\n");
  HddCount      = 0;
  BbsCount      = 0;
  LocalHddInfo  = NULL;
  LocalBbsTable = NULL;

  Status        = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (mBootOptionBbsMapping != NULL) {
    FreePool (mBootOptionBbsMapping);

    mBootOptionBbsMapping      = NULL;
    mBootOptionBbsMappingCount = 0;
  }

  LegacyBios->GetBbsInfo (
                LegacyBios,
                &HddCount,
                &LocalHddInfo,
                &BbsCount,
                &LocalBbsTable
                );

  BootOrder = BdsLibGetVariableAndSize (
                L"BootOrder",
                &gEfiGlobalVariableGuid,
                &BootOrderSize
                );
  if (BootOrder == NULL) {
    BootOrderSize = 0;
  }

  for (Index = 0; Index < BbsCount; Index++) {
    if ((LocalBbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) ||
        (LocalBbsTable[Index].BootPriority == BBS_DO_NOT_BOOT_FROM)
        ) {
      continue;
    }

    BdsBuildLegacyDevNameString (&LocalBbsTable[Index], Index, sizeof (Desc), Desc);

    Exist = BdsFindLegacyBootOptionByDevTypeAndName (
              BootOrder,
              BootOrderSize / sizeof (UINT16),
              LocalBbsTable[Index].DeviceType,
              Desc,
              &Attribute,
              &BbsIndex,
              &OptionNumber
              );
    Print(L"Boot entry #%d %s exist.\n", Index, Exist ? L"does" : L"does not");
    if (!Exist) {
      //
      // Not found such type of legacy device in boot options or we found but it's disabled
      // so we have to create one and put it to the tail of boot order list
      //
      Status = BdsCreateOneLegacyBootOption (
                &LocalBbsTable[Index],
                Index,
                &BootOrder,
                &BootOrderSize
                );
      if (EFI_ERROR (Status)) {
        break;
      }
      BbsIndex     = Index;
      OptionNumber = BootOrder[BootOrderSize / sizeof (UINT16) - 1];
    }

    ASSERT (BbsIndex == Index);
    //
    // Save the BbsIndex
    //
    mBootOptionBbsMapping = ReallocatePool (
                              mBootOptionBbsMappingCount * sizeof (BOOT_OPTION_BBS_MAPPING),
                              (mBootOptionBbsMappingCount + 1) * sizeof (BOOT_OPTION_BBS_MAPPING),
                              mBootOptionBbsMapping
                              );
    ASSERT (mBootOptionBbsMapping != NULL);
    mBootOptionBbsMapping[mBootOptionBbsMappingCount].BootOptionNumber = OptionNumber;
    mBootOptionBbsMapping[mBootOptionBbsMappingCount].BbsIndex         = Index;
    mBootOptionBbsMapping[mBootOptionBbsMappingCount].BbsType          = LocalBbsTable[Index].DeviceType;
    mBootOptionBbsMappingCount ++;
  }

  //
  // Group the Boot Option Number in BootOrder for the same type devices
  //
  GroupMultipleLegacyBootOption4SameType (
    BootOrder,
    BootOrderSize / sizeof (UINT16)
    );

  if (BootOrderSize > 0) {
    Status = gRT->SetVariable (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    BootOrderSize,
                    BootOrder
                    );
  } else {
    EfiLibDeleteVariable (L"BootOrder", &gEfiGlobalVariableGuid);
  }

  if (BootOrder != NULL) {
    FreePool (BootOrder);
  }

  Print(L"Exiting BdsAddNonExistingLegacyBootOptions()\n");

  return Status;
}

/**
  Deletete the Boot Option from EFI Variable. The Boot Order Arrray
  is also updated.

  @param OptionNumber    The number of Boot option want to be deleted.
  @param BootOrder       The Boot Order array.
  @param BootOrderSize   The size of the Boot Order Array.

  @retval  EFI_SUCCESS           The Boot Option Variable was found and removed
  @retval  EFI_UNSUPPORTED       The Boot Option Variable store was inaccessible
  @retval  EFI_NOT_FOUND         The Boot Option Variable was not found
**/
EFI_STATUS
EFIAPI
BdsDeleteBootOption (
  IN UINTN                       OptionNumber,
  IN OUT UINT16                  *BootOrder,
  IN OUT UINTN                   *BootOrderSize
  )
{
  UINT16      BootOption[100];
  UINTN       Index;
  EFI_STATUS  Status;
  UINTN       Index2Del;

  Status    = EFI_SUCCESS;
  Index2Del = 0;

  UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", OptionNumber);
  Status = EfiLibDeleteVariable (BootOption, &gEfiGlobalVariableGuid);

  //
  // adjust boot order array
  //
  for (Index = 0; Index < *BootOrderSize / sizeof (UINT16); Index++) {
    if (BootOrder[Index] == OptionNumber) {
      Index2Del = Index;
      break;
    }
  }

  if (Index != *BootOrderSize / sizeof (UINT16)) {
    for (Index = 0; Index < *BootOrderSize / sizeof (UINT16) - 1; Index++) {
      if (Index >= Index2Del) {
        BootOrder[Index] = BootOrder[Index + 1];
      }
    }

    *BootOrderSize -= sizeof (UINT16);
  }

  return Status;

}

/**
  Delete all the invalid legacy boot options.

  @retval EFI_SUCCESS             All invalide legacy boot options are deleted.
  @retval EFI_OUT_OF_RESOURCES    Fail to allocate necessary memory.
  @retval EFI_NOT_FOUND           Fail to retrive variable of boot order.
**/
EFI_STATUS
EFIAPI
BdsDeleteAllInvalidLegacyBootOptions (
  VOID
  )
{
  UINT16                    *BootOrder;
  UINT8                     *BootOptionVar;
  UINTN                     BootOrderSize;
  UINTN                     BootOptionSize;
  EFI_STATUS                Status;
  UINT16                    HddCount;
  UINT16                    BbsCount;
  HDD_INFO                  *LocalHddInfo;
  BBS_TABLE                 *LocalBbsTable;
  BBS_TABLE                 *BbsEntry;
  UINT16                    BbsIndex;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  UINTN                     Index;
  UINT16                    BootOption[10];
  UINT16                    BootDesc[100];
  BOOLEAN                   DescStringMatch;

  Status        = EFI_SUCCESS;
  BootOrder     = NULL;
  BootOrderSize = 0;
  HddCount      = 0;
  BbsCount      = 0;
  LocalHddInfo  = NULL;
  LocalBbsTable = NULL;
  BbsEntry      = NULL;

  Status        = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  LegacyBios->GetBbsInfo (
                LegacyBios,
                &HddCount,
                &LocalHddInfo,
                &BbsCount,
                &LocalBbsTable
                );

  BootOrder = BdsLibGetVariableAndSize (
                L"BootOrder",
                &gEfiGlobalVariableGuid,
                &BootOrderSize
                );
  if (BootOrder == NULL) {
    BootOrderSize = 0;
  }

  Index = 0;
  while (Index < BootOrderSize / sizeof (UINT16)) {
    UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", BootOrder[Index]);
    BootOptionVar = BdsLibGetVariableAndSize (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      &BootOptionSize
                      );
    if (NULL == BootOptionVar) {
      BootOptionSize = 0;
      Status = gRT->GetVariable (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      NULL,
                      &BootOptionSize,
                      BootOptionVar
                      );
      if (Status == EFI_NOT_FOUND) {
        //
        // Update BootOrder
        //
        Print(L"About to call BdsDeleteBootOption() on Index == %d\n", Index);
        BdsDeleteBootOption (
          BootOrder[Index],
          BootOrder,
          &BootOrderSize
          );
        continue;
      } else {
        FreePool (BootOrder);
        return EFI_OUT_OF_RESOURCES;
      }
    }

    //
    // Skip Non-Legacy boot option
    // 
    if (!BdsIsLegacyBootOption (BootOptionVar, &BbsEntry, &BbsIndex)) {
      if (BootOptionVar!= NULL) {
        FreePool (BootOptionVar);
      }
      Index++;
      continue;
    }

    if (BbsIndex < BbsCount) {
      //
      // Check if BBS Description String is changed
      //
      DescStringMatch = FALSE;
      BdsBuildLegacyDevNameString (
        &LocalBbsTable[BbsIndex],
        BbsIndex,
        sizeof (BootDesc),
        BootDesc
        );

      if (StrCmp (BootDesc, (UINT16*)(BootOptionVar + sizeof (UINT32) + sizeof (UINT16))) == 0) {
        DescStringMatch = TRUE;
      }

      if (!((LocalBbsTable[BbsIndex].BootPriority == BBS_IGNORE_ENTRY) ||
            (LocalBbsTable[BbsIndex].BootPriority == BBS_DO_NOT_BOOT_FROM)) &&
          (LocalBbsTable[BbsIndex].DeviceType == BbsEntry->DeviceType) &&
          DescStringMatch) {
        Index++;
        continue;
      }
    }

    if (BootOptionVar != NULL) {
      FreePool (BootOptionVar);
    }
    //
    // should delete
    //
    Print(L"Second About to call BdsDeleteBootOption(), Index = %d\n", Index);
    BdsDeleteBootOption (
      BootOrder[Index],
      BootOrder,
      &BootOrderSize
      );
  }

  //
  // Adjust the number of boot options.
  //
  if (BootOrderSize != 0) {
    Status = gRT->SetVariable (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    BootOrderSize,
                    BootOrder
                    );
  } else {
    EfiLibDeleteVariable (L"BootOrder", &gEfiGlobalVariableGuid);
  }

  if (BootOrder != NULL) {
    FreePool (BootOrder);
  }

  return Status;
}

/**
  Fill the device order buffer.

  @param BbsTable        The BBS table.
  @param BbsType         The BBS Type.
  @param BbsCount        The BBS Count.
  @param Buf             device order buffer.

  @return The device order buffer.

**/
UINT16 *
BdsFillDevOrderBuf (
  IN BBS_TABLE                    *BbsTable,
  IN BBS_TYPE                     BbsType,
  IN UINTN                        BbsCount,
  OUT UINT16                      *Buf
  )
{
  UINTN Index;

  for (Index = 0; Index < BbsCount; Index++) {
    if (BbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) {
      continue;
    }

    if (BbsTable[Index].DeviceType != BbsType) {
      continue;
    }

    *Buf = (UINT16) (Index & 0xFF);
    Buf++;
  }

  return Buf;
}

/**
  Create the device order buffer.

  @param BbsTable        The BBS table.
  @param BbsCount        The BBS Count.

  @retval EFI_SUCCES             The buffer is created and the EFI variable named 
                                 VAR_LEGACY_DEV_ORDER and gEfiLegacyDevOrderVariableGuid is
                                 set correctly.
  @retval EFI_OUT_OF_RESOURCES   Memmory or storage is not enough.
  @retval EFI_DEVICE_ERROR       Fail to add the device order into EFI variable fail
                                 because of hardware error.
**/
EFI_STATUS
BdsCreateDevOrder (
  IN BBS_TABLE                  *BbsTable,
  IN UINT16                     BbsCount
  )
{
  UINTN                       Index;
  UINTN                       FDCount;
  UINTN                       HDCount;
  UINTN                       CDCount;
  UINTN                       NETCount;
  UINTN                       BEVCount;
  UINTN                       TotalSize;
  UINTN                       HeaderSize;
  LEGACY_DEV_ORDER_ENTRY      *DevOrder;
  LEGACY_DEV_ORDER_ENTRY      *DevOrderPtr;
  EFI_STATUS                  Status;

  FDCount     = 0;
  HDCount     = 0;
  CDCount     = 0;
  NETCount    = 0;
  BEVCount    = 0;
  TotalSize   = 0;
  HeaderSize  = sizeof (BBS_TYPE) + sizeof (UINT16);
  DevOrder    = NULL;
  Status      = EFI_SUCCESS;

  //
  // Count all boot devices
  //
  for (Index = 0; Index < BbsCount; Index++) {
    if (BbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) {
      continue;
    }

    switch (BbsTable[Index].DeviceType) {
    case BBS_FLOPPY:
      FDCount++;
      break;

    case BBS_HARDDISK:
      HDCount++;
      break;

    case BBS_CDROM:
      CDCount++;
      break;

    case BBS_EMBED_NETWORK:
      NETCount++;
      break;

    case BBS_BEV_DEVICE:
      BEVCount++;
      break;

    default:
      break;
    }
  }

  TotalSize += (HeaderSize + sizeof (UINT16) * FDCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * HDCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * CDCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * NETCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * BEVCount);

  //
  // Create buffer to hold all boot device order
  //
  DevOrder = AllocateZeroPool (TotalSize);
  if (NULL == DevOrder) {
    return EFI_OUT_OF_RESOURCES;
  }
  DevOrderPtr          = DevOrder;

  DevOrderPtr->BbsType = BBS_FLOPPY;
  DevOrderPtr->Length  = (UINT16) (sizeof (DevOrderPtr->Length) + FDCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *) BdsFillDevOrderBuf (BbsTable, BBS_FLOPPY, BbsCount, DevOrderPtr->Data);

  DevOrderPtr->BbsType = BBS_HARDDISK;
  DevOrderPtr->Length  = (UINT16) (sizeof (UINT16) + HDCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *) BdsFillDevOrderBuf (BbsTable, BBS_HARDDISK, BbsCount, DevOrderPtr->Data);
  
  DevOrderPtr->BbsType = BBS_CDROM;
  DevOrderPtr->Length  = (UINT16) (sizeof (UINT16) + CDCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *) BdsFillDevOrderBuf (BbsTable, BBS_CDROM, BbsCount, DevOrderPtr->Data);
  
  DevOrderPtr->BbsType = BBS_EMBED_NETWORK;
  DevOrderPtr->Length  = (UINT16) (sizeof (UINT16) + NETCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *) BdsFillDevOrderBuf (BbsTable, BBS_EMBED_NETWORK, BbsCount, DevOrderPtr->Data);

  DevOrderPtr->BbsType = BBS_BEV_DEVICE;
  DevOrderPtr->Length  = (UINT16) (sizeof (UINT16) + BEVCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *) BdsFillDevOrderBuf (BbsTable, BBS_BEV_DEVICE, BbsCount, DevOrderPtr->Data);

  ASSERT (TotalSize == (UINTN) ((UINT8 *) DevOrderPtr - (UINT8 *) DevOrder));

  //
  // Save device order for legacy boot device to variable.
  //
  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &gEfiLegacyDevOrderVariableGuid,
                  VAR_FLAG,
                  TotalSize,
                  DevOrder
                  );
  FreePool (DevOrder);

  return Status;
}

/**
  Add the legacy boot devices from BBS table into 
  the legacy device boot order.

  @retval EFI_SUCCESS           The boot devices are added successfully.
  @retval EFI_NOT_FOUND         The legacy boot devices are not found.
  @retval EFI_OUT_OF_RESOURCES  Memmory or storage is not enough.
  @retval EFI_DEVICE_ERROR      Fail to add the legacy device boot order into EFI variable
                                because of hardware error.
**/
EFI_STATUS
EFIAPI
BdsUpdateLegacyDevOrder (
  VOID
  )
{
  LEGACY_DEV_ORDER_ENTRY      *DevOrder;
  LEGACY_DEV_ORDER_ENTRY      *NewDevOrder;
  LEGACY_DEV_ORDER_ENTRY      *Ptr;
  LEGACY_DEV_ORDER_ENTRY      *NewPtr;
  UINTN                       DevOrderSize;
  EFI_LEGACY_BIOS_PROTOCOL    *LegacyBios;
  EFI_STATUS                  Status;
  UINT16                      HddCount;
  UINT16                      BbsCount;
  HDD_INFO                    *LocalHddInfo;
  BBS_TABLE                   *LocalBbsTable;
  INTN                        Index;
  INTN                        Index2;
  UINTN                       *Idx;
  UINTN                       FDCount;
  UINTN                       HDCount;
  UINTN                       CDCount;
  UINTN                       NETCount;
  UINTN                       BEVCount;
  UINTN                       TotalSize;
  UINTN                       HeaderSize;
  UINT16                      *NewFDPtr;
  UINT16                      *NewHDPtr;
  UINT16                      *NewCDPtr;
  UINT16                      *NewNETPtr;
  UINT16                      *NewBEVPtr;
  UINT16                      *NewDevPtr;
  UINTN                       FDIndex;
  UINTN                       HDIndex;
  UINTN                       CDIndex;
  UINTN                       NETIndex;
  UINTN                       BEVIndex;

  Print(L"Entering BdsUpdateLegacyDevOrder()\n");

  Idx           = NULL;
  FDCount       = 0;
  HDCount       = 0;
  CDCount       = 0;
  NETCount      = 0;
  BEVCount      = 0;
  TotalSize     = 0;
  HeaderSize    = sizeof (BBS_TYPE) + sizeof (UINT16);
  FDIndex       = 0;
  HDIndex       = 0;
  CDIndex       = 0;
  NETIndex      = 0;
  BEVIndex      = 0;
  NewDevPtr     = NULL;

  Print(L"About to call EfiLibLocateProtocol()\n");
  Status        = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Print(L"About to call GetBbsInfo()\n");
  Status = LegacyBios->GetBbsInfo (
                         LegacyBios,
                         &HddCount,
                         &LocalHddInfo,
                         &BbsCount,
                         &LocalBbsTable
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Print(L"About to call BdsLibGetVariableAndSize()\n");
  DevOrder = BdsLibGetVariableAndSize (
               VAR_LEGACY_DEV_ORDER,
               &gEfiLegacyDevOrderVariableGuid,
               &DevOrderSize
               );
  if (NULL == DevOrder) {
    Print(L"DevOrder was NULL\n");
    return BdsCreateDevOrder (LocalBbsTable, BbsCount);
  }
  //
  // First we figure out how many boot devices with same device type respectively
  //
  Print(L"About to enter for() loop, BbsCount = %d\n", BbsCount);
  for (Index = 0; Index < BbsCount; Index++) {
//    Print(L" Loop: %d\n", Index);
    if ((LocalBbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) ||
        (LocalBbsTable[Index].BootPriority == BBS_DO_NOT_BOOT_FROM)
        ) {
      continue;
    }

    switch (LocalBbsTable[Index].DeviceType) {
    case BBS_FLOPPY:
      FDCount++;
      break;

    case BBS_HARDDISK:
      HDCount++;
      break;

    case BBS_CDROM:
      CDCount++;
      break;

    case BBS_EMBED_NETWORK:
      NETCount++;
      break;

    case BBS_BEV_DEVICE:
      BEVCount++;
      break;

    default:
      break;
    }
  }

  Print(L"Done with for() loop; counted:\n Floppies: %d\n Hard disks: %d\n CD-ROMs: %d\n", FDCount, HDCount, CDCount);
  TotalSize += (HeaderSize + FDCount * sizeof (UINT16));
  TotalSize += (HeaderSize + HDCount * sizeof (UINT16));
  TotalSize += (HeaderSize + CDCount * sizeof (UINT16));
  TotalSize += (HeaderSize + NETCount * sizeof (UINT16));
  TotalSize += (HeaderSize + BEVCount * sizeof (UINT16));

  Print(L"About to allocate memory for NewDevOrder\n");
  NewDevOrder = AllocateZeroPool (TotalSize);
  if (NULL == NewDevOrder) {
    return EFI_OUT_OF_RESOURCES;
  }


  Print(L"About to copy FD entries\n");
  //
  // copy FD
  //
  Ptr             = DevOrder;
  NewPtr          = NewDevOrder;
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16) (sizeof (UINT16) + FDCount * sizeof (UINT16));
  Print(L" Copying FD entries; about to loop from 0 to %d\n", Ptr->Length / sizeof (UINT16) - 1);
  for (Index = 0; Index < (INTN) (Ptr->Length / sizeof (UINT16) - 1); Index++) {
//    Print(L"  Looping: Index = %d\n", Index);
    if (LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].DeviceType != BBS_FLOPPY
        ) {
      continue;
    }

//    Print(L"  About to set data\n");
    NewPtr->Data[FDIndex] = Ptr->Data[Index];
    FDIndex++;
  }
  Print(L" Done with for() loop\n");
  NewFDPtr = NewPtr->Data;

  Print(L"About to copy HD entries\n");
  //
  // copy HD
  //
  Ptr             = (LEGACY_DEV_ORDER_ENTRY *) (&Ptr->Data[Ptr->Length / sizeof (UINT16) - 1]);
  NewPtr          = (LEGACY_DEV_ORDER_ENTRY *) (&NewPtr->Data[NewPtr->Length / sizeof (UINT16) -1]);
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (sizeof (UINT16) + HDCount * sizeof (UINT16));
  for (Index = 0; Index < (INTN) (Ptr->Length / sizeof (UINT16) - 1); Index++) {
    if (LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_LOWEST_PRIORITY ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].DeviceType != BBS_HARDDISK
        ) {
      continue;
    }

    NewPtr->Data[HDIndex] = Ptr->Data[Index];
    HDIndex++;
  }
  NewHDPtr = NewPtr->Data;

  Print(L"About to copy CD entries\n");
  //
  // copy CD
  //
  Ptr    = (LEGACY_DEV_ORDER_ENTRY *) (&Ptr->Data[Ptr->Length / sizeof (UINT16) - 1]);
  NewPtr = (LEGACY_DEV_ORDER_ENTRY *) (&NewPtr->Data[NewPtr->Length / sizeof (UINT16) -1]);
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16) (sizeof (UINT16) + CDCount * sizeof (UINT16));
  for (Index = 0; Index < (INTN) (Ptr->Length / sizeof (UINT16) - 1); Index++) {
    if (LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_LOWEST_PRIORITY ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].DeviceType != BBS_CDROM
        ) {
      continue;
    }

    NewPtr->Data[CDIndex] = Ptr->Data[Index];
    CDIndex++;
  }
  NewCDPtr = NewPtr->Data;

  //
  // copy NET
  //
  Ptr    = (LEGACY_DEV_ORDER_ENTRY *) (&Ptr->Data[Ptr->Length / sizeof (UINT16) - 1]);
  NewPtr = (LEGACY_DEV_ORDER_ENTRY *) (&NewPtr->Data[NewPtr->Length / sizeof (UINT16) -1]);
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16) (sizeof (UINT16) + NETCount * sizeof (UINT16));
  for (Index = 0; Index < (INTN) (Ptr->Length / sizeof (UINT16) - 1); Index++) {
    if (LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_LOWEST_PRIORITY ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].DeviceType != BBS_EMBED_NETWORK
        ) {
      continue;
    }

    NewPtr->Data[NETIndex] = Ptr->Data[Index];
    NETIndex++;
  }
  NewNETPtr = NewPtr->Data;
  
  //
  // copy BEV
  //
  Ptr    = (LEGACY_DEV_ORDER_ENTRY *) (&Ptr->Data[Ptr->Length / sizeof (UINT16) - 1]);
  NewPtr = (LEGACY_DEV_ORDER_ENTRY *) (&NewPtr->Data[NewPtr->Length / sizeof (UINT16) -1]);
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16) (sizeof (UINT16) + BEVCount * sizeof (UINT16));
  for (Index = 0; Index < (INTN) (Ptr->Length / sizeof (UINT16) - 1); Index++) {
    if (LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].BootPriority == BBS_LOWEST_PRIORITY ||
        LocalBbsTable[Ptr->Data[Index] & 0xFF].DeviceType != BBS_BEV_DEVICE
        ) {
      continue;
    }

    NewPtr->Data[BEVIndex] = Ptr->Data[Index];
    BEVIndex++;
  }
  NewBEVPtr = NewPtr->Data;

  Print(L"About to enter another loop; BbsCount = %d\n", BbsCount);
  for (Index = 0; Index < BbsCount; Index++) {
//    Print(L" Looping: Index = %d\n", Index);
    if ((LocalBbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) ||
        (LocalBbsTable[Index].BootPriority == BBS_DO_NOT_BOOT_FROM)
        ) {
      continue;
    }

    switch (LocalBbsTable[Index].DeviceType) {
    case BBS_FLOPPY:
      Idx       = &FDIndex;
      NewDevPtr = NewFDPtr;
      break;

    case BBS_HARDDISK:
      Idx       = &HDIndex;
      NewDevPtr = NewHDPtr;
      break;

    case BBS_CDROM:
      Idx       = &CDIndex;
      NewDevPtr = NewCDPtr;
      break;

    case BBS_EMBED_NETWORK:
      Idx       = &NETIndex;
      NewDevPtr = NewNETPtr;
      break;

    case BBS_BEV_DEVICE:
      Idx       = &BEVIndex;
      NewDevPtr = NewBEVPtr;
      break;

    default:
      Idx = NULL;
      break;
    }
    //
    // at this point we have copied those valid indexes to new buffer
    // and we should check if there is any new appeared boot device
    //
    Print(L" Checking for new devices\n");
    if (Idx != NULL) {
      for (Index2 = 0; Index2 < *Idx; Index2++) {
        if ((NewDevPtr[Index2] & 0xFF) == (UINT16) Index) {
          break;
        }
      }

      if (Index2 == *Idx) {
        //
        // Index2 == *Idx means we didn't find Index
        // so Index is a new appeared device's index in BBS table
        // insert it before disabled indexes.
        //
        for (Index2 = 0; Index2 < *Idx; Index2++) {
          if ((NewDevPtr[Index2] & 0xFF00) == 0xFF00) {
            break;
          }
        }
        CopyMem (&NewDevPtr[Index2 + 1], &NewDevPtr[Index2], (*Idx - Index2) * sizeof (UINT16));
        NewDevPtr[Index2] = (UINT16) (Index & 0xFF);
        (*Idx)++;
      }
    }
  }

  FreePool (DevOrder);

  Print(L"About to call SetVariable()\n");
  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &gEfiLegacyDevOrderVariableGuid,
                  VAR_FLAG,
                  TotalSize,
                  NewDevOrder
                  );
  FreePool (NewDevOrder);

  Print(L"Returning from BdsUpdateLegacyDevOrder()\n");
  return Status;
}

