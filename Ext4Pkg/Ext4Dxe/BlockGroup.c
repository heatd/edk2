/**
 * @brief Block group related routines
 *
 * Copyright (c) 2021 Pedro Falcato All rights reserved.
 * 
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Ext4.h"


EFI_STATUS Ext4ReadInode(EXT4_PARTITION *Partition, EXT4_INO_NR InodeNum, EXT4_INODE **OutIno)
{
    UINT64 InodeOffset;
    UINTN BlockGroupNumber = (UINTN) DivU64x64Remainder(InodeNum - 1,
                                                Partition->SuperBlock.s_inodes_per_group,
                                                &InodeOffset);

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
        DEBUG((EFI_D_INFO, "Error st %x; inode offset %lx inode table start %lu block group %lu\n", st, InodeOffset,
                            InodeTableStart, BlockGroupNumber));
        FreePool(Inode);
        return st;
    }

    *OutIno = Inode;
    return EFI_SUCCESS;
}
