/**
 * @file Common header for the driver
 * 
 * @copyright Copyright (c) 2021 Pedro Falcato
 * 
 */

#ifndef _EXT2_H
#define _EXT2_H

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

#include "Ext2Disk.h"

#define EXT2_DRIVER_VERSION    0x0000

EFI_STATUS Ext2OpenPartition(EFI_DISK_IO_PROTOCOL *diskIo, EFI_DISK_IO2_PROTOCOL *diskIo2,
                             EFI_BLOCK_IO_PROTOCOL *blockIo);

typedef struct _EXT2_PARTITION
{
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL Interface;
  EFI_DISK_IO_PROTOCOL *diskIo;
  EFI_DISK_IO2_PROTOCOL *diskIo2;
  EFI_BLOCK_IO_PROTOCOL *blockIo;

  EXT2_SUPERBLOCK SuperBlock;

  UINT32 FeaturesIncompat;
  UINT32 FeaturesCompat;
  UINT32 FeaturesRoCompat;
  UINT32 InodeSize;
  UINT32 BlockSize;
  BOOLEAN ReadOnly;
} EXT2_PARTITION;

EFI_STATUS Ext2OpenSuperblock(EXT2_PARTITION *Partition);

static inline EFI_BLOCK_IO_PROTOCOL *Ext2BlockIo(EXT2_PARTITION *Partition)
{
  return Partition->blockIo;
}

static inline EFI_DISK_IO_PROTOCOL *Ext2DiskIo(EXT2_PARTITION *Partition)
{
  return Partition->diskIo;
}

static inline EFI_DISK_IO2_PROTOCOL *Ext2DiskIo2(EXT2_PARTITION *Partition)
{
  return Partition->diskIo2;
}

static inline UINT32 Ext2MediaId(EXT2_PARTITION *Partition)
{
  return Partition->blockIo->Media->MediaId;
}

EFI_STATUS Ext2ReadDiskIo(EXT2_PARTITION *Partition, void *Buffer, UINTN Length, UINTN Offset);

#endif
