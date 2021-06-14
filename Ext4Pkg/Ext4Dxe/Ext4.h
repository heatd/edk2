/**
 * @file Common header for the driver
 * 
 * @copyright Copyright (c) 2021 Pedro Falcato
 * 
 */

#ifndef _Ext4_H
#define _Ext4_H

#include <Uefi.h>

#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "Ext4Disk.h"

#define Ext4_DRIVER_VERSION    0x0000

EFI_STATUS Ext4OpenPartition(EFI_DISK_IO_PROTOCOL *diskIo, EFI_DISK_IO2_PROTOCOL *diskIo2,
                             EFI_BLOCK_IO_PROTOCOL *blockIo);

typedef struct _Ext4_PARTITION
{
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL Interface;
  EFI_DISK_IO_PROTOCOL *diskIo;
  EFI_DISK_IO2_PROTOCOL *diskIo2;
  EFI_BLOCK_IO_PROTOCOL *blockIo;

  EXT4_SUPERBLOCK SuperBlock;

  UINT32 FeaturesIncompat;
  UINT32 FeaturesCompat;
  UINT32 FeaturesRoCompat;
  UINT32 InodeSize;
  UINT32 BlockSize;
  BOOLEAN ReadOnly;
} EXT4_PARTITION;

EFI_STATUS Ext4OpenSuperblock(EXT4_PARTITION *Partition);

static inline EFI_BLOCK_IO_PROTOCOL *Ext4BlockIo(EXT4_PARTITION *Partition)
{
  return Partition->blockIo;
}

static inline EFI_DISK_IO_PROTOCOL *Ext4DiskIo(EXT4_PARTITION *Partition)
{
  return Partition->diskIo;
}

static inline EFI_DISK_IO2_PROTOCOL *Ext4DiskIo2(EXT4_PARTITION *Partition)
{
  return Partition->diskIo2;
}

static inline UINT32 Ext4MediaId(EXT4_PARTITION *Partition)
{
  return Partition->blockIo->Media->MediaId;
}

EFI_STATUS Ext4ReadDiskIo(EXT4_PARTITION *Partition, void *Buffer, UINTN Length, UINTN Offset);

#endif
