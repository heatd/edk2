/**
 * @file Superblock managing routines
 * 
 * @copyright Copyright (c) 2021 Pedro Falcato
 * 
 */

#include "Ext4.h"
#include "Ext4Disk.h"
#include "Uefi/UefiBaseType.h"

#include <Library/DebugLib.h>

static const UINT32 supported_compat_feat = EXT4_FEATURE_COMPAT_EXT_ATTR;

static const UINT32 supported_ro_compat_feat =
  EXT4_FEATURE_RO_COMPAT_DIR_NLINK | EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE |
  EXT4_FEATURE_RO_COMPAT_HUGE_FILE | EXT4_FEATURE_RO_COMPAT_LARGE_FILE |
  EXT4_FEATURE_RO_COMPAT_GDT_CSUM | EXT4_FEATURE_RO_COMPAT_METADATA_CSUM | EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER;
// TODO: Add btree support
static const UINT32 supported_incompat_feat =
  EXT4_FEATURE_INCOMPAT_64BIT | EXT4_FEATURE_INCOMPAT_DIRDATA |
  EXT4_FEATURE_INCOMPAT_FLEX_BG | EXT4_FEATURE_INCOMPAT_FILETYPE |
  EXT4_FEATURE_INCOMPAT_EXTENTS | EXT4_FEATURE_INCOMPAT_LARGEDIR |
  EXT4_FEATURE_INCOMPAT_MMP;

// TODO: Add meta_bg support

// Note: We ignore MMP because it's impossible that it's mapped elsewhere,
// I think (unless there's some sort of network setup where we're accessing a remote partition).

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
  // accidentally opening an ext2/3/4 filesystem we don't understand, which would be disasterous.

  if(Partition->FeaturesIncompat & ~supported_incompat_feat)
  {
    DEBUG((EFI_D_INFO, "[Ext4] Unsupported %lx\n", Partition->FeaturesIncompat & ~supported_incompat_feat));
    return EFI_UNSUPPORTED;
  }
  
  UINT32 unsupported_ro_compat = Partition->FeaturesRoCompat & ~supported_ro_compat_feat; 
  if(unsupported_ro_compat != 0)
  {
    DEBUG((EFI_D_INFO, "[Ext4] Unsupported ro compat %lx\n", unsupported_ro_compat));
    Partition->ReadOnly = TRUE;
  }

  (void) supported_compat_feat;

  DEBUG((EFI_D_INFO, "Read only = %u\n", Partition->ReadOnly));

  Partition->BlockSize = 1024 << sb->s_log_block_size;
  Partition->NumberBlocks = Ext4MakeBlockNumberFromHalfs(Partition, sb->s_blocks_count, sb->s_blocks_count_hi);
  Partition->NumberBlockGroups = Partition->NumberBlocks / sb->s_blocks_per_group;

  DEBUG((EFI_D_INFO, "[ext4] Number of blocks = %lu\n[ext4] Number of block groups: %lu\n",
         Partition->NumberBlocks, Partition->NumberBlockGroups));
  
  if(Ext4Is64Bit(Partition))
  {
    Partition->DescSize = sb->s_desc_size;
  }
  else
  {
    Partition->DescSize = EXT4_OLD_BLOCK_DESC_SIZE;
  }

  EXT4_BLOCK_NR NrBlocks = Partition->NumberBlockGroups * Partition->DescSize;
  Partition->BlockGroups = Ext4AllocAndReadBlocks(Partition, NrBlocks, Partition->BlockSize == 1024 ? 2 : 1);

  if(!Partition->BlockGroups)
    return EFI_OUT_OF_RESOURCES;
  
  // Note that the cast below is completely safe, because EXT4_FILE is a specialisation of EFI_FILE_PROTOCOL
  st = Ext4OpenVolume(&Partition->Interface, (EFI_FILE_PROTOCOL **) &Partition->Root);
  DEBUG((EFI_D_INFO, "[ext4] Root File %p\n", Partition->Root));
  return st;
}
