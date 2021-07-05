/**
 * @brief Block group related routines
 *
 * @copyright Copyright (c) 2021 Pedro Falcato
 */

#include "Ext4.h"
#include "Library/MemoryAllocationLib.h"
#include "Uefi/UefiBaseType.h"

#include <Uefi.h>

EFI_STATUS Ext4ReadInode(EXT4_PARTITION *Partition, EXT4_INO_NR InodeNum, EXT4_INODE **OutIno)
{
    UINTN BlockGroupNumber = (InodeNum - 1) / Partition->SuperBlock.s_inodes_per_group;
    UINTN InodeOffset = (InodeNum - 1) % Partition->SuperBlock.s_inodes_per_group;

    EXT4_INODE *Inode = AllocatePool(Partition->InodeSize);
    if (!Inode) {
        return EFI_OUT_OF_RESOURCES;
    }

    EXT4_BLOCK_GROUP_DESC *BlockGroup = Ext4GetBlockGroupDesc(Partition, BlockGroupNumber);

    // TODO: INODE_UNINIT

    EXT4_BLOCK_NR InodeTableStart = Ext4MakeBlockNumberFromHalfs(Partition, BlockGroup->bg_inode_table_lo,
                                                                 BlockGroup->bg_inode_table_hi);
    
    EFI_STATUS st = Ext4ReadDiskIo(Partition, Inode, Partition->InodeSize,
                   Ext4BlockToByteOffset(Partition, InodeTableStart) + InodeOffset * Partition->InodeSize);
    if (EFI_ERROR(st)) {
        FreePool(Inode);
        return st;
    }

    *OutIno = Inode;
    return EFI_SUCCESS;
}
