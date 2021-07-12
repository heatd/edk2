/**
 * @file Block group related routines
 *
 * Copyright (c) 2021 Pedro Falcato All rights reserved.
 * Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Ext4.h"

/**
   Reads an inode from disk.
   
   @param[in]    Partition  Pointer to the opened partition. 
   @param[in]    InodeNum   Number of the desired Inode 
   @param[out]   OutIno     Pointer to where it will be stored a pointer to the read inode.

   @retval EFI_STATUS       Status of the inode read. 
 */
EFI_STATUS
Ext4ReadInode (
  IN EXT4_PARTITION *Partition, IN EXT4_INO_NR InodeNum, OUT EXT4_INODE **OutIno
  )
{
  UINT64  InodeOffset;
  UINT32  BlockGroupNumber = (UINT32)DivU64x64Remainder (
                                       InodeNum - 1,
                                       Partition->SuperBlock.s_inodes_per_group,
                                       &InodeOffset
                                       );

  // Check for the block group number's correctness
  if (BlockGroupNumber >= Partition->NumberBlockGroups) {
    return EFI_VOLUME_CORRUPTED;
  }

  EXT4_INODE  *Inode = Ext4AllocateInode (Partition);

  if (Inode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EXT4_BLOCK_GROUP_DESC  *BlockGroup = Ext4GetBlockGroupDesc (Partition, BlockGroupNumber);

  // Note: We'll need to check INODE_UNINIT and friends when we add write support

  EXT4_BLOCK_NR  InodeTableStart = Ext4MakeBlockNumberFromHalfs (
                                     Partition,
                                     BlockGroup->bg_inode_table_lo,
                                     BlockGroup->bg_inode_table_hi
                                     );

  EFI_STATUS  st = Ext4ReadDiskIo (
                     Partition,
                     Inode,
                     Partition->InodeSize,
                     Ext4BlockToByteOffset (Partition, InodeTableStart) + InodeOffset * Partition->InodeSize
                     );

  if (EFI_ERROR (st)) {
    DEBUG ((
      EFI_D_ERROR,
      "[ext4] Error reading inode: st %x; inode offset %lx"
      " inode table start %lu block group %lu\n",
      st,
      InodeOffset,
      InodeTableStart,
      BlockGroupNumber
      ));
    FreePool (Inode);
    return st;
  }

  if (!Ext4CheckInodeChecksum (Partition, Inode, InodeNum)) {
    DEBUG ((
      EFI_D_ERROR,
      "[ext4] Inode %llu has invalid checksum (calculated %x)\n",
      InodeNum,
      Ext4CalculateInodeChecksum (Partition, Inode, InodeNum)
      ));
    FreePool (Inode);
    return EFI_VOLUME_CORRUPTED;
  }

  *OutIno = Inode;
  return EFI_SUCCESS;
}
