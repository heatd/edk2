/**
 * @file Common header for the driver
 * 
 * @copyright Copyright (c) 2021 Pedro Falcato
 * 
 */

#ifndef _EXT4_H
#define _EXT4_H

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

#define EXT4_NAME_MAX          255

#define EXT4_DRIVER_VERSION    0x0000

EFI_STATUS Ext4OpenPartition(EFI_HANDLE DeviceHandle, EFI_DISK_IO_PROTOCOL *diskIo, EFI_DISK_IO2_PROTOCOL *diskIo2,
                             EFI_BLOCK_IO_PROTOCOL *blockIo);

typedef struct _Ext4File EXT4_FILE;

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
  UINT64 NumberBlockGroups;
  EXT4_BLOCK_NR NumberBlocks;

  EXT4_BLOCK_GROUP_DESC *BlockGroups;
  UINT32 DescSize;
  EXT4_FILE *Root;

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

EFI_STATUS Ext4ReadDiskIo(EXT4_PARTITION *Partition, void *Buffer, UINTN Length, UINT64 Offset);

EFI_STATUS Ext4ReadBlocks(EXT4_PARTITION *Partition, void *Buffer, UINTN NumberBlocks, EXT4_BLOCK_NR BlockNumber);

void *Ext4AllocAndReadBlocks(EXT4_PARTITION *Partition, UINTN NumberBlocks, EXT4_BLOCK_NR BlockNumber);

static inline BOOLEAN Ext4Is64Bit(const EXT4_PARTITION *Partition)
{
  return Partition->FeaturesIncompat & EXT4_FEATURE_INCOMPAT_64BIT;
}

static inline EXT4_BLOCK_NR Ext4MakeBlockNumberFromHalfs(const EXT4_PARTITION *Partition, UINT32 Low, UINT32 High)
{
  // High might have garbage if it's not a 64 bit filesystem
  return Ext4Is64Bit(Partition) ? Low | ((UINT64) High << 32) : Low;
}

static inline EXT4_BLOCK_GROUP_DESC *Ext4GetBlockGroupDesc(EXT4_PARTITION *Partition, UINT32 BlockGroup)
{
  return (EXT4_BLOCK_GROUP_DESC *) ((char *) Partition->BlockGroups + BlockGroup * Partition->DescSize);
}

EFI_STATUS Ext4ReadInode(EXT4_PARTITION *Partition, EXT4_INO_NR InodeNum, EXT4_INODE **OutIno);

static inline UINT64 Ext4BlockToByteOffset(const EXT4_PARTITION *Partition, EXT4_BLOCK_NR Block)
{
  return Partition->BlockSize * Block;
}

EFI_STATUS Ext4Read(EXT4_PARTITION *Partition, EXT4_INODE *Inode, void *Buffer, UINT64 Offset, IN OUT UINT64 *Length);

static inline UINT64 Ext4InodeSize(EXT4_INODE *Inode)
{
  return ((UINT64) Inode->i_size_hi << 32) | Inode->i_size_lo;
}

EFI_STATUS Ext4GetExtent(EXT4_PARTITION *Partition, EXT4_INODE *Inode, EXT4_BLOCK_NR LogicalBlock, OUT EXT4_EXTENT *Extent); 

struct _Ext4File
{
  EFI_FILE_PROTOCOL Protocol;
  EXT4_INODE *Inode;
  EXT4_INO_NR InodeNum;

  UINT64 OpenMode;
  UINT64 Position;

  EXT4_PARTITION *Partition;
};

EFI_STATUS Ext4RetrieveDirent(EXT4_FILE *File, const CHAR16 *NameUnicode, EXT4_PARTITION *Partition,
			OUT EXT4_DIR_ENTRY *res);

EFI_STATUS Ext4OpenFile(EXT4_FILE *Directory, const CHAR16 *Name, EXT4_PARTITION *Partition, UINT64 OpenMode,
                        OUT EXT4_FILE **OutFile);

EXT4_INODE *Ext4AllocateInode(EXT4_PARTITION *Partition);


// Part of the EFI_SIMPLE_FILE_SYSTEM_PROTOCOL

EFI_STATUS EFIAPI Ext4OpenVolume(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Partition, EFI_FILE_PROTOCOL **Root);

void Ext4SetupFile(IN OUT EXT4_FILE *File, IN EXT4_PARTITION *Partition);

EFI_STATUS Ext4CloseInternal(
  IN EXT4_FILE *File
  );

// Part of the EFI_FILE_PROTOCOL

EFI_STATUS EFIAPI Ext4Open(
  IN EFI_FILE_PROTOCOL        *This,
  OUT EFI_FILE_PROTOCOL       **NewHandle,
  IN CHAR16                   *FileName,
  IN UINT64                   OpenMode,
  IN UINT64                   Attributes
  );

EFI_STATUS EFIAPI Ext4Close(
  IN EFI_FILE_PROTOCOL *This
  );

EFI_STATUS EFIAPI Ext4Delete(
  IN EFI_FILE_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI Ext4ReadFile(
  IN EFI_FILE_PROTOCOL        *This,
  IN OUT UINTN                *BufferSize,
  OUT VOID                    *Buffer
  );

EFI_STATUS
EFIAPI Ext4WriteFile(
  IN EFI_FILE_PROTOCOL        *This,
  IN OUT UINTN                *BufferSize,
  IN VOID                    *Buffer
  );

EFI_STATUS
EFIAPI Ext4GetPosition(
  IN EFI_FILE_PROTOCOL        *This,
  OUT UINT64                  *Position
  );

EFI_STATUS
EFIAPI Ext4SetPosition(
  IN EFI_FILE_PROTOCOL        *This,
  IN UINT64                   Position
  );

EFI_STATUS
EFIAPI Ext4GetInfo(
  IN EFI_FILE_PROTOCOL        *This,
  IN EFI_GUID                 *InformationType,
  IN OUT UINTN                *BufferSize,
  OUT VOID                    *Buffer
  );

// EFI_FILE_PROTOCOL implementation ends here.

BOOLEAN Ext4FileIsDir(IN const EXT4_FILE *File);

BOOLEAN Ext4FileIsReg(IN const EXT4_FILE *File);

// In EFI we can't open FIFO pipes, UNIX sockets, character/block devices since these concepts are
// at the kernel level and are OS dependent.
static inline BOOLEAN Ext4FileIsOpenable(IN const EXT4_FILE *File)
{
  return Ext4FileIsReg(File) || Ext4FileIsDir(File);
}

UINT64 Ext4FilePhysicalSpace(EXT4_FILE *File);

void Ext4FileATime(IN EXT4_FILE *File, OUT EFI_TIME *Time);
void Ext4FileMTime(IN EXT4_FILE *File, OUT EFI_TIME *Time);
void Ext4FileCreateTime(IN EXT4_FILE *File, OUT EFI_TIME *Time);


/**
   Initialises Unicode collation, which is needed for case-insensitive string comparisons
   within the driver (a good example of an application of this is filename comparison).
 
   @param[in]      DriverHandle    Handle to the driver image.
 
   @retval EFI_SUCCESS   Unicode collation was successfully initialised.
   @retval !EFI_SUCCESS  Failure.
*/
EFI_STATUS Ext4InitialiseUnicodeCollation(EFI_HANDLE DriverHandle);

/**
   Does a case-insensitive string comparison. Refer to EFI_UNICODE_COLLATION_PROTOCOL's StriColl
   for more details.
 
   @param[in]      Str1   Pointer to a null terminated string.
   @param[in]      Str2   Pointer to a null terminated string.
 
   @retval 0   Str1 is equivalent to Str2.
   @retval >0  Str1 is lexically greater than Str2.
   @retval <0  Str1 is lexically less than Str2.
*/
INTN Ext4StrCmpInsensitive(
  IN CHAR16                                 *Str1,
  IN CHAR16                                 *Str2
  );

#endif
