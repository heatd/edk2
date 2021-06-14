/**
 * @file Superblock managing routines
 * 
 * @copyright Copyright (c) 2021 Pedro Falcato
 * 
 */

#include "Ext4.h"

BOOLEAN Ext4SuperblockValidate(EXT4_SUPERBLOCK *sb)
{
  if(sb->s_magic != EXT4_SIGNATURE)
    return FALSE;
  
  if(sb->s_rev_level != EXT4_DYNAMIC_REV && sb->s_rev_level != EXT4_GOOD_OLD_REV)
    return FALSE;

  return TRUE;
}

EFI_STATUS Ext4OpenSuperblock(EXT4_PARTITION *Partition)
{
	EFI_STATUS st = Ext4ReadDiskIo(Partition,
	                               &Partition->SuperBlock,
                                 sizeof(EXT4_SUPERBLOCK),
                                 EXT4_SUPERBLOCK_OFFSET);
  
  if(EFI_ERROR(st))
    return st;
  
  EXT4_SUPERBLOCK *sb = &Partition->SuperBlock;

  if(!Ext4SuperblockValidate(sb))
    return EFI_UNSUPPORTED;
  
  if(sb->s_rev_level == EXT4_DYNAMIC_REV)
  {
    Partition->FeaturesCompat = sb->s_feature_compat;
    Partition->FeaturesIncompat = sb->s_feature_incompat;
    Partition->FeaturesRoCompat = sb->s_feature_ro_compat;
    Partition->InodeSize = sb->s_inode_size;
  }
  else
  {
    // GOOD_OLD_REV
    Partition->FeaturesCompat = Partition->FeaturesIncompat = Partition->FeaturesRoCompat = 0;
    Partition->InodeSize = EXT4_GOOD_OLD_INODE_SIZE;
  }

  // Now, check for the feature set of the filesystem
  // It's essential to check for this to avoid filesystem corruption and to avoid
  // accidentally opening an ext3/4 filesystem, which would be disasterous.

  if(Partition->FeaturesIncompat & ~EXT4_SUPPORTED_INCOMPAT)
    return EFI_UNSUPPORTED;
  
  if(Partition->FeaturesRoCompat & ~EXT4_SUPPORTED_RO_COMPAT)
  {
    Partition->ReadOnly = TRUE;
  }

  Partition->BlockSize = 1024 << sb->s_log_block_size;
  
  return EFI_SUCCESS;
}
