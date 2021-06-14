/**
 * @file Disk utilities
 * 
 * @copyright Copyright (c) 2021 Pedro Falcato
 * 
 */
#include "Ext4.h"

EFI_STATUS Ext4ReadDiskIo(EXT4_PARTITION *Partition, void *Buffer, UINTN Length, UINTN Offset)
{
	return Ext4DiskIo(Partition)->ReadDisk(Ext4DiskIo(Partition), Ext4MediaId(Partition), Offset, Length, Buffer);
}
