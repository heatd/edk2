/**
 * @brief Inode related routines
 *
 * Copyright (c) 2021 Pedro Falcato All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Ext4.h"

#include <Uefi.h>
#include <Library/TimeBaseLib.h>

EFI_STATUS
Ext4Read (
  EXT4_PARTITION *Partition, EXT4_FILE *File, void *Buffer, UINT64 Offset, IN OUT UINTN *Length
  )
{
  // DEBUG((EFI_D_INFO, "Ext4Read[Offset %lu, Length %lu]\n", Offset, *Length));
  EXT4_INODE  *Inode    = File->Inode;
  UINT64      InodeSize = Ext4InodeSize (Inode);

  if(Offset > InodeSize) {
    return EFI_DEVICE_ERROR;
  }

  UINT64  CurrentSeek   = Offset;
  UINTN   RemainingRead = *Length;
  UINTN   BeenRead = 0;

  if (RemainingRead > InodeSize - Offset) {
    RemainingRead = (UINTN)(InodeSize - Offset);
  }

  while(RemainingRead != 0) {
    UINTN        WasRead = 0;
    EXT4_EXTENT  Extent;

    // The algorithm here is to get the extent corresponding to the current block
    // and then read as much as we can from the current extent.
    UINT32  BlockOff;

    EFI_STATUS  st = Ext4GetExtent (
                       Partition,
                       File,
                       DivU64x32Remainder (CurrentSeek, Partition->BlockSize, &BlockOff),
                       &Extent
                       );

    if(st != EFI_SUCCESS && st != EFI_NO_MAPPING) {
      return st;
    }

    BOOLEAN  HasBackingExtent = st != EFI_NO_MAPPING;

    if(!HasBackingExtent) {
      UINT32  HoleOff = BlockOff;
      UINTN   HoleLen = Partition->BlockSize - HoleOff;
      WasRead = HoleLen > RemainingRead ? RemainingRead : HoleLen;
      // TODO: Get the hole size and memset all that
      SetMem (Buffer, WasRead, 0);
    } else {
      UINT64  ExtentStartBytes   = (((UINT64)Extent.ee_start_hi << 32) | Extent.ee_start_lo) * Partition->BlockSize;
      UINT64  ExtentLengthBytes  = Extent.ee_len * Partition->BlockSize;
      UINT64  ExtentLogicalBytes = (UINT64)Extent.ee_block * Partition->BlockSize;

      // Our extent offset is the difference between CurrentSeek and ExtentLogicalBytes
      UINT64  ExtentOffset  = CurrentSeek - ExtentLogicalBytes;
      UINTN   ExtentMayRead = (UINTN)(ExtentLengthBytes - ExtentOffset);
      WasRead = ExtentMayRead > RemainingRead ? RemainingRead : ExtentMayRead;

      // DEBUG((EFI_D_INFO, "[ext4] may read %lu, remaining %lu\n", ExtentMayRead, RemainingRead));
      // DEBUG((EFI_D_INFO, "[ext4] Reading block %lu\n", (ExtentStartBytes + ExtentOffset) / Partition->BlockSize));
      st = Ext4ReadDiskIo (Partition, Buffer, WasRead, ExtentStartBytes + ExtentOffset);

      if(EFI_ERROR (st)) {
        DEBUG ((
          EFI_D_ERROR,
          "[ext4] Error %x reading [%lu, %lu]\n",
          st,
          ExtentStartBytes + ExtentOffset,
          ExtentStartBytes + ExtentOffset + WasRead - 1
          ));
        return st;
      }
    }

    RemainingRead -= WasRead;
    Buffer       = (VOID *)((CHAR8 *)Buffer + WasRead);
    BeenRead    += WasRead;
    CurrentSeek += WasRead;
  }

  *Length = BeenRead;

  // DEBUG((EFI_D_INFO, "File length %lu crc %x\n", BeenRead, CalculateCrc32(original, BeenRead)));
  return EFI_SUCCESS;
}

EXT4_INODE *
Ext4AllocateInode (
  EXT4_PARTITION *Partition
  )
{
  return AllocateZeroPool (Partition->BlockSize);
}

BOOLEAN
Ext4FileIsDir (
  IN const EXT4_FILE *File
  )
{
  return (File->Inode->i_mode & EXT4_INO_TYPE_DIR) == EXT4_INO_TYPE_DIR;
}

BOOLEAN
Ext4FileIsReg (
  IN const EXT4_FILE *File
  )
{
  return (File->Inode->i_mode & EXT4_INO_TYPE_REGFILE) == EXT4_INO_TYPE_REGFILE;
}

UINT64
Ext4FilePhysicalSpace (
  EXT4_FILE *File
  )
{
  BOOLEAN  HugeFile = File->Partition->FeaturesIncompat & EXT4_FEATURE_RO_COMPAT_HUGE_FILE;
  UINT64   Blocks   = File->Inode->i_blocks;

  if(HugeFile) {
    Blocks |= ((UINT64)File->Inode->i_osd2.data_linux.l_i_blocks_high) << 32;

    // If HUGE_FILE is enabled and EXT4_HUGE_FILE_FL is set in the inode's flags, each unit
    // in i_blocks corresponds to an actual filesystem block
    if(File->Inode->i_flags & EXT4_HUGE_FILE_FL) {
      return Blocks * File->Partition->BlockSize;
    }
  }

  // Else, each i_blocks unit corresponds to 512 bytes
  return Blocks * 512;
}

#define EXT4_EXTRA_TIMESTAMP_MASK  ((1 << 2) - 1)

#define EXT4_FILE_GET_TIME_GENERIC(Name, Field)            \
  void Ext4File ## Name (IN EXT4_FILE *File, OUT EFI_TIME *Time) \
  {                                                          \
    EXT4_INODE  *Inode = File->Inode;                       \
    UINT64      SecondsEpoch = Inode->Field;                   \
    UINT32      Nanoseconds  = 0;                                \
                                                           \
    if (Ext4InodeHasField (Inode, Field ## _extra)) {          \
      SecondsEpoch |= ((UINT64)(Inode->Field ## _extra & EXT4_EXTRA_TIMESTAMP_MASK)) << 32; \
      Nanoseconds   = Inode->Field ## _extra >> 2;                                            \
    }                                                                                       \
    EpochToEfiTime ((UINTN)SecondsEpoch, Time);                                                     \
    Time->Nanosecond = Nanoseconds;                                                         \
  }

// Note: EpochToEfiTime should be adjusted to take in a UINT64 instead of a UINTN, in order to avoid Y2038
// on 32-bit systems.

EXT4_FILE_GET_TIME_GENERIC (ATime, i_atime);
EXT4_FILE_GET_TIME_GENERIC (MTime, i_mtime);
static EXT4_FILE_GET_TIME_GENERIC (CrTime, i_crtime);

void
Ext4FileCreateTime (
  IN EXT4_FILE *File, OUT EFI_TIME *Time
  )
{
  EXT4_INODE  *Inode = File->Inode;

  if (!Ext4InodeHasField (Inode, i_crtime)) {
    SetMem (Time, sizeof (EFI_TIME), 0);
    return;
  }

  Ext4FileCrTime (File, Time);
}
