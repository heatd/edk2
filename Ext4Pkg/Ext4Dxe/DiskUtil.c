/**
 * @file Disk utilities
 * 
 * @copyright Copyright (c) 2021 Pedro Falcato
 * 
 */
#include "Ext4.h"
#include "Library/MemoryAllocationLib.h"

EFI_STATUS Ext4ReadDiskIo(EXT4_PARTITION *Partition, void *Buffer, UINTN Length, UINT64 Offset)
{
	return Ext4DiskIo(Partition)->ReadDisk(Ext4DiskIo(Partition), Ext4MediaId(Partition), Offset, Length, Buffer);
}

EFI_STATUS Ext4ReadBlocks(EXT4_PARTITION *Partition, void *Buffer, UINTN NumberBlocks, EXT4_BLOCK_NR BlockNumber)
{
	return Ext4ReadDiskIo(Partition, Buffer, NumberBlocks * Partition->BlockSize, BlockNumber * Partition->BlockSize);
}

void *Ext4AllocAndReadBlocks(EXT4_PARTITION *Partition, UINTN NumberBlocks, EXT4_BLOCK_NR BlockNumber)
{
	void *Buf = AllocatePool(NumberBlocks * Partition->BlockSize);

	if(!Buf)
		return NULL;

	if(Ext4ReadBlocks(Partition, Buf, NumberBlocks, BlockNumber) != EFI_SUCCESS)
	{
		FreePool(Buf);
		return NULL;
	}

	return Buf;
}
