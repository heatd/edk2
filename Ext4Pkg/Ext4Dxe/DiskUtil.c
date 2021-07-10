/**
 * @file Disk utilities
 *
 * Copyright (c) 2021 Pedro Falcato All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#include "Ext4.h"

EFI_STATUS
Ext4ReadDiskIo (
  EXT4_PARTITION *Partition, VOID *Buffer, UINTN Length, UINT64 Offset
  )
{
  return Ext4DiskIo (Partition)->ReadDisk (Ext4DiskIo (Partition), Ext4MediaId (Partition), Offset, Length, Buffer);
}

EFI_STATUS
Ext4ReadBlocks (
  EXT4_PARTITION *Partition, VOID *Buffer, UINTN NumberBlocks, EXT4_BLOCK_NR BlockNumber
  )
{
  return Ext4ReadDiskIo (Partition, Buffer, NumberBlocks * Partition->BlockSize, BlockNumber * Partition->BlockSize);
}

VOID *
Ext4AllocAndReadBlocks (
  EXT4_PARTITION *Partition, UINTN NumberBlocks, EXT4_BLOCK_NR BlockNumber
  )
{
  VOID  *Buf = AllocatePool (NumberBlocks * Partition->BlockSize);

  if(Buf == NULL) {
    return NULL;
  }

  if(Ext4ReadBlocks (Partition, Buf, NumberBlocks, BlockNumber) != EFI_SUCCESS) {
    FreePool (Buf);
    return NULL;
  }

  return Buf;
}
