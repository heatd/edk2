/**
 * @file Superblock managing routines
 * 
 * @copyright Copyright (c) 2021 Pedro Falcato
 * 
 */

#include "Ext2.h"

BOOLEAN Ext2SuperblockValidate(EXT2_SUPERBLOCK *sb)
{
  if(sb->s_magic != EXT2_SIGNATURE)
    return FALSE;
  
  if(sb->s_rev_level != EXT2_DYNAMIC_REV && sb->s_rev_level != EXT2_GOOD_OLD_REV)
    return FALSE;

  return TRUE;
}

EFI_STATUS Ext2OpenSuperblock(EXT2_PARTITION *Partition)
{
	EFI_STATUS st = Ext2ReadDiskIo(Partition,
	                               &Partition->SuperBlock,
                                 sizeof(EXT2_SUPERBLOCK),
                                 EXT2_SUPERBLOCK_OFFSET);
  
  if(EFI_ERROR(st))
    return st;
  
  EXT2_SUPERBLOCK *sb = &Partition->SuperBlock;

  if(!Ext2SuperblockValidate(sb))
    return EFI_UNSUPPORTED;
  
  if(sb->s_rev_level == EXT2_DYNAMIC_REV)
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
    Partition->InodeSize = EXT2_GOOD_OLD_INODE_SIZE;
  }

  // Now, check for the feature set of the filesystem
  // It's essential to check for this to avoid filesystem corruption and to avoid
  // accidentally opening an ext3/4 filesystem, which would be disasterous.

  if(Partition->FeaturesIncompat & ~EXT2_SUPPORTED_INCOMPAT)
    return EFI_UNSUPPORTED;
  
  if(Partition->FeaturesRoCompat & ~EXT2_SUPPORTED_RO_COMPAT)
  {
    Partition->ReadOnly = TRUE;
  }

  Partition->BlockSize = 1024 << sb->s_log_block_size;
  
  return EFI_SUCCESS;
}
