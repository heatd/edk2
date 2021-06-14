/**
 * @file Driver entry point
 * 
 * @copyright Copyright (c) 2021 Pedro Falcato
 * 
 */

#include "Ext4.h"

EFI_STATUS Ext4OpenPartition(EFI_DISK_IO_PROTOCOL *diskIo, EFI_DISK_IO2_PROTOCOL *diskIo2,
                             EFI_BLOCK_IO_PROTOCOL *blockIo)
{
	EXT4_PARTITION *part = AllocateZeroPool(sizeof(*part));
	if(!part)
		return EFI_OUT_OF_RESOURCES;
	
	part->blockIo = blockIo;
	part->diskIo = diskIo;
	part->diskIo2 = diskIo2;

	EFI_STATUS st = Ext4OpenSuperblock(part);

	if(EFI_ERROR(st))
	{
		FreePool(part);
		return st;
	}

	return EFI_SUCCESS;
}
