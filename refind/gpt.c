/*
 * refind/gpt.c
 * Functions related to GPT data structures
 *
 * Copyright (c) 2014 Roderick W. Smith
 * All rights reserved.
 *
 * This program is distributed under the terms of the GNU General Public
 * License (GPL) version 3 (GPLv3), a copy of which must be distributed
 * with this source code or binaries made from it.
 *
 */

#include "gpt.h"
#include "lib.h"
#include "screen.h"
#include "../include/refit_call_wrapper.h"

#ifdef __MAKEWITH_TIANO
#define BlockIoProtocol gEfiBlockIoProtocolGuid
#endif

extern GPT_DATA *gPartitions;

GPT_DATA * AllocateGptData(VOID) {
   GPT_DATA *GptData;

   GptData = AllocateZeroPool(sizeof(GPT_DATA));
   if (GptData != NULL) {
      GptData->ProtectiveMBR = AllocateZeroPool(sizeof(MBR_RECORD));
      GptData->Header = AllocateZeroPool(sizeof(GPT_HEADER));
      if ((GptData->ProtectiveMBR == NULL) || (GptData->Header == NULL)) {
         MyFreePool(GptData->ProtectiveMBR);
         MyFreePool(GptData->Header);
         MyFreePool(GptData);
         GptData = NULL;
      } // if
   } // if
   return GptData;
} // GPT_DATA * AllocateGptData()

VOID ClearGptData(GPT_DATA *Data) {
   if (Data) {
      if (Data->ProtectiveMBR)
         MyFreePool(Data->ProtectiveMBR);
      if (Data->Header)
         MyFreePool(Data->Header);
      if (Data->Entries)
         MyFreePool(Data->Entries);
      MyFreePool(Data);
   } // if
} // VOID ClearGptData()

// TODO: Add more tests, like a CRC check.
// Returns TRUE if the GPT header data appear valid, FALSE otherwise.
static BOOLEAN GptHeaderValid(GPT_HEADER *Header) {
   BOOLEAN IsValid = TRUE;

   if ((Header->signature != 0x5452415020494645ULL) || (Header->entry_count > 2048))
      IsValid = FALSE;

   return IsValid;
} // BOOLEAN GptHeaderValid

// Read GPT data from Volume and store it in Data. Note that this function
// may be called on a Volume that is not in fact the protective MBR of a GPT
// disk, in which case it will return EFI_LOAD_ERROR or some other error
// condition. In this case, *Data will be left alone.
EFI_STATUS ReadGptData(REFIT_VOLUME *Volume, GPT_DATA **Data) {
   EFI_STATUS Status = EFI_SUCCESS;
   UINT64     BufferSize;
   GPT_DATA   *GptData; // Temporary holding storage; transferred to *Data later

   if ((Volume == NULL) || (Data == NULL))
      Status = EFI_INVALID_PARAMETER;

   // get block i/o
   if ((Status == EFI_SUCCESS) && (Volume->BlockIO == NULL)) {
      Status = refit_call3_wrapper(BS->HandleProtocol, Volume->DeviceHandle, &BlockIoProtocol, (VOID **) &(Volume->BlockIO));
      if (EFI_ERROR(Status)) {
         Volume->BlockIO = NULL;
         Print(L"Warning: Can't get BlockIO protocol.\n");
         Status = EFI_INVALID_PARAMETER;
      }
   } // if

   if ((Status == EFI_SUCCESS) && ((!Volume->BlockIO->Media->MediaPresent) || (Volume->BlockIO->Media->LogicalPartition)))
      Status = EFI_NO_MEDIA;

   if (Status == EFI_SUCCESS) {
      GptData = AllocateGptData();
      if (GptData == NULL) {
         Status = EFI_OUT_OF_RESOURCES;
      } // if
   } // if

   // Read the MBR and store it in GptData->ProtectiveMBR.
   if (Status == EFI_SUCCESS) {
      Status = refit_call5_wrapper(Volume->BlockIO->ReadBlocks, Volume->BlockIO, Volume->BlockIO->Media->MediaId,
                                   0, 512, (VOID*) GptData->ProtectiveMBR);
   }

   // If it looks like a valid protective MBR, try to do more with it....
   if ((Status == EFI_SUCCESS) && (GptData->ProtectiveMBR->MBRSignature == 0xAA55) &&
       ((GptData->ProtectiveMBR->partitions[0].type == 0xEE) || (GptData->ProtectiveMBR->partitions[1].type == 0xEE) ||
        (GptData->ProtectiveMBR->partitions[2].type = 0xEE) || (GptData->ProtectiveMBR->partitions[3].type == 0xEE))) {
      Status = refit_call5_wrapper(Volume->BlockIO->ReadBlocks, Volume->BlockIO, Volume->BlockIO->Media->MediaId,
                                   1, sizeof(GPT_HEADER), GptData->Header);

      // Do basic sanity check on GPT header.
      if ((Status == EFI_SUCCESS) && !GptHeaderValid(GptData->Header)) {
         Status = EFI_UNSUPPORTED;
      }

      // Load actual GPT table....
      if (Status == EFI_SUCCESS) {
         BufferSize = GptData->Header->entry_count * 128;
         if (GptData->Entries != NULL)
            MyFreePool(GptData->Entries);
         GptData->Entries = AllocatePool(BufferSize);
         if (GptData->Entries == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
         } // if
      } // if

      if (Status == EFI_SUCCESS) {
         Status = refit_call5_wrapper(Volume->BlockIO->ReadBlocks, Volume->BlockIO, Volume->BlockIO->Media->MediaId,
                                      GptData->Header->header_lba, BufferSize, GptData->Entries);
      } // if

   } else {
      Status = EFI_LOAD_ERROR;
   } // if/else

   if (Status == EFI_SUCCESS) {
      // Everything looks OK, so copy it over
      ClearGptData(*Data);
      *Data = GptData;
   } else {
      ClearGptData(GptData);
   } // if/else

   return Status;
} // EFI_STATUS ReadGptData()

// Look in gPartitions for a partition with the specified Guid. If found, return
// a pointer to that partition's name string. If not found, return a NULL pointer.
// The calling function is responsible for freeing the returned memory.
CHAR16 * PartNameFromGuid(EFI_GUID *Guid) {
   UINTN     i;
   CHAR16    *Found = NULL;
   GPT_DATA  *GptData;

   if ((Guid == NULL) || (gPartitions == NULL))
      return NULL;

   GptData = gPartitions;
   while ((GptData != NULL) && (!Found)) {
      i = 0;
      while ((i < GptData->Header->entry_count) && (!Found)) {
         if (GuidsAreEqual((EFI_GUID*) &(GptData->Entries[i].partition_guid), Guid))
            Found = StrDuplicate(GptData->Entries[i].name);
         else
            i++;
      } // while(scanning entries)
      GptData = GptData->NextEntry;
   } // while(scanning GPTs)
   return Found;
} // CHAR16 * PartNameFromGuid()

// Erase the gPartitions linked-list data structure
VOID ForgetPartitionTables(VOID) {
   GPT_DATA  *Next;

   while (gPartitions != NULL) {
      Next = gPartitions->NextEntry;
      ClearGptData(gPartitions);
      gPartitions = Next;
   } // while
} // VOID ForgetPartitionTables()

// If Volume points to a whole disk with a GPT, add it to the gPartitions
// linked list of GPTs.
VOID AddPartitionTable(REFIT_VOLUME *Volume) {
   GPT_DATA    *GptData = NULL, *GptList;
   EFI_STATUS  Status;

   Status = ReadGptData(Volume, &GptData);
   if (Status == EFI_SUCCESS) {
      if (gPartitions == NULL) {
         gPartitions = GptData;
      } else {
         GptList = gPartitions;
         while (GptList->NextEntry != NULL) {
            GptList = GptList->NextEntry;
         } // while
         GptList->NextEntry = GptData;
      } // if/else
   } else if (GptData != NULL) {
      ClearGptData(GptData);
   } // if/else
} // VOID AddPartitionTable()

