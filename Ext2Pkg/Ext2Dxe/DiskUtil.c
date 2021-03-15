/**
 * @file Disk utilities
 * 
 * @copyright Copyright (c) 2021 Pedro Falcato
 * 
 */
#include "Ext2.h"

EFI_STATUS Ext2ReadDiskIo(EXT2_PARTITION *Partition, void *Buffer, UINTN Length, UINTN Offset)
{
	return Ext2DiskIo(Partition)->ReadDisk(Ext2DiskIo(Partition), Ext2MediaId(Partition), Offset, Length, Buffer);
}
