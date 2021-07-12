/**
 * @file Superblock managing routines
 *
 * Copyright (c) 2021 Pedro Falcato All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Ext4.h"
#include "Uefi/UefiBaseType.h"

STATIC CONST UINT32  supported_compat_feat = EXT4_FEATURE_COMPAT_EXT_ATTR;

STATIC CONST UINT32  supported_ro_compat_feat =
  EXT4_FEATURE_RO_COMPAT_DIR_NLINK | EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE |
  EXT4_FEATURE_RO_COMPAT_HUGE_FILE | EXT4_FEATURE_RO_COMPAT_LARGE_FILE |
  EXT4_FEATURE_RO_COMPAT_GDT_CSUM | EXT4_FEATURE_RO_COMPAT_METADATA_CSUM | EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER;
// TODO: Add btree support
STATIC CONST UINT32  supported_incompat_feat =
  EXT4_FEATURE_INCOMPAT_64BIT | EXT4_FEATURE_INCOMPAT_DIRDATA |
  EXT4_FEATURE_INCOMPAT_FLEX_BG | EXT4_FEATURE_INCOMPAT_FILETYPE |
  EXT4_FEATURE_INCOMPAT_EXTENTS | EXT4_FEATURE_INCOMPAT_LARGEDIR |
  EXT4_FEATURE_INCOMPAT_MMP;

// TODO: Add meta_bg support

// Note: We ignore MMP because it's impossible that it's mapped elsewhere,
// I think (unless there's some sort of network setup where we're accessing a remote partition).

BOOLEAN
Ext4SuperblockValidate (
  EXT4_SUPERBLOCK *sb
  )
{
  if(sb->s_magic != EXT4_SIGNATURE) {
    return FALSE;
  }

  // TODO: We should try to support EXT2/3 partitions too
  if(sb->s_rev_level != EXT4_DYNAMIC_REV && sb->s_rev_level != EXT4_GOOD_OLD_REV) {
    return FALSE;
  }

  // TODO: Is this correct behaviour? Imagine the power cuts out, should the system fail to boot because
  // we're scared of touching something corrupt?
  if((sb->s_state & EXT4_FS_STATE_UNMOUNTED) == 0) {
    return FALSE;
  }

  return TRUE;
}

STATIC UINT32
Ext4CalculateSuperblockChecksum (
  EXT4_PARTITION *Partition, CONST EXT4_SUPERBLOCK *sb
  )
{
  // Most checksums require us to go through a dummy 0 as part of the requirement
  // that the checksum is done over a structure with its checksum field = 0.
  UINT32  Checksum = Ext4CalculateChecksum (
                       Partition,
                       sb,
                       OFFSET_OF (EXT4_SUPERBLOCK, s_checksum),
                       ~0U
                       );

  return Checksum;
}

STATIC BOOLEAN
Ext4VerifySuperblockChecksum (
  EXT4_PARTITION *Partition, CONST EXT4_SUPERBLOCK *sb
  )
{
  return sb->s_checksum == Ext4CalculateSuperblockChecksum (Partition, sb);
}

/**
 * Opens and parses the superblock.
 *
 * @param[out]     Partition Partition structure to fill with filesystem details.
 * @retval EFI_STATUS        EFI_SUCCESS if parsing was succesful and the partition is a
                             valid ext4 partition.
 */
EFI_STATUS
Ext4OpenSuperblock (
  OUT EXT4_PARTITION *Partition
  )
{
  UINT32      Index;
  EFI_STATUS  Status;
  EXT4_SUPERBLOCK  *Sb;

  Status = Ext4ReadDiskIo (
             Partition,
             &Partition->SuperBlock,
             sizeof (EXT4_SUPERBLOCK),
             EXT4_SUPERBLOCK_OFFSET
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  Sb = &Partition->SuperBlock;

  if (!Ext4SuperblockValidate (Sb)) {
    return EFI_VOLUME_CORRUPTED;
  }

  if (Sb->s_rev_level == EXT4_DYNAMIC_REV) {
    Partition->FeaturesCompat   = Sb->s_feature_compat;
    Partition->FeaturesIncompat = Sb->s_feature_incompat;
    Partition->FeaturesRoCompat = Sb->s_feature_ro_compat;
    Partition->InodeSize = Sb->s_inode_size;
  } else {
    // GOOD_OLD_REV
    Partition->FeaturesCompat = Partition->FeaturesIncompat = Partition->FeaturesRoCompat = 0;
    Partition->InodeSize = EXT4_GOOD_OLD_INODE_SIZE;
  }

  // Now, check for the feature set of the filesystem
  // It's essential to check for this to avoid filesystem corruption and to avoid
  // accidentally opening an ext2/3/4 filesystem we don't understand, which would be disasterous.

  if (Partition->FeaturesIncompat & ~supported_incompat_feat) {
    DEBUG ((EFI_D_INFO, "[Ext4] Unsupported %lx\n", Partition->FeaturesIncompat & ~supported_incompat_feat));
    return EFI_UNSUPPORTED;
  }

  // At the time of writing, it's the only supported checksum.
  if (Partition->FeaturesCompat & EXT4_FEATURE_RO_COMPAT_METADATA_CSUM &&
      Sb->s_checksum_type != EXT4_CHECKSUM_CRC32C) {
    return EFI_UNSUPPORTED;
  }

  if (Partition->FeaturesIncompat & EXT4_FEATURE_INCOMPAT_CSUM_SEED) {
    Partition->InitialSeed = Sb->s_checksum_seed;
  } else {
    Partition->InitialSeed = Ext4CalculateChecksum (Partition, Sb->s_uuid, 16, ~0U);
  }

  UINT32  unsupported_ro_compat = Partition->FeaturesRoCompat & ~supported_ro_compat_feat;

  if (unsupported_ro_compat != 0) {
    DEBUG ((EFI_D_INFO, "[Ext4] Unsupported ro compat %x\n", unsupported_ro_compat));
    Partition->ReadOnly = TRUE;
  }

  (VOID)supported_compat_feat;

  DEBUG ((EFI_D_INFO, "Read only = %u\n", Partition->ReadOnly));

  Partition->BlockSize = 1024 << Sb->s_log_block_size;

  // The size of a block group can also be calculated as 8 * Partition->BlockSize
  if(Sb->s_blocks_per_group != 8 * Partition->BlockSize) {
    return EFI_UNSUPPORTED;
  }

  Partition->NumberBlocks = Ext4MakeBlockNumberFromHalfs (Partition, Sb->s_blocks_count, Sb->s_blocks_count_hi);
  Partition->NumberBlockGroups = DivU64x32 (Partition->NumberBlocks, Sb->s_blocks_per_group);

  DEBUG ((
    EFI_D_INFO,
    "[ext4] Number of blocks = %lu\n[ext4] Number of block groups: %lu\n",
    Partition->NumberBlocks,
    Partition->NumberBlockGroups
    ));

  if (Ext4Is64Bit (Partition)) {
    Partition->DescSize = Sb->s_desc_size;
  } else {
    Partition->DescSize = EXT4_OLD_BLOCK_DESC_SIZE;
  }

  if (Partition->DescSize < EXT4_64BIT_BLOCK_DESC_SIZE && Ext4Is64Bit (Partition)) {
    // 64 bit filesystems need DescSize to be 64 bytes
    return EFI_VOLUME_CORRUPTED;
  }

  if (!Ext4VerifySuperblockChecksum (Partition, Sb)) {
    DEBUG ((EFI_D_ERROR, "[ext4] Bad superblock checksum %lx\n", Ext4CalculateSuperblockChecksum (Partition, Sb)));
    return EFI_VOLUME_CORRUPTED;
  }

  UINT32  NrBlocksRem;
  UINTN   NrBlocks = (UINTN)DivU64x32Remainder (
                              Partition->NumberBlockGroups * Partition->DescSize,
                              Partition->BlockSize,
                              &NrBlocksRem
                              );

  if(NrBlocksRem != 0) {
    NrBlocks++;
  }

  Partition->BlockGroups = Ext4AllocAndReadBlocks (Partition, NrBlocks, Partition->BlockSize == 1024 ? 2 : 1);

  if (!Partition->BlockGroups) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < Partition->NumberBlockGroups; Index++) {
    EXT4_BLOCK_GROUP_DESC *Desc = Ext4GetBlockGroupDesc(Partition, Index);
    if (!Ext4VerifyBlockGroupDescChecksum(Partition, Desc, Index)) {
      DEBUG ((EFI_D_INFO, "[ext4] Block group descriptor %u has an invalid checksum\n", Index));
      return EFI_VOLUME_CORRUPTED;
    }
  }
  
  // Note that the cast below is completely safe, because EXT4_FILE is a specialisation of EFI_FILE_PROTOCOL
  Status = Ext4OpenVolume (&Partition->Interface, (EFI_FILE_PROTOCOL **)&Partition->Root);

  DEBUG ((EFI_D_INFO, "[ext4] Root File %p\n", Partition->Root));
  return Status;
}

/**
   Calculates the checksum of the given buffer.
   @param[in]      Partition     Pointer to the opened EXT4 partition.
   @param[in]      Buffer        Pointer to the buffer.
   @param[in]      Length        Length of the buffer, in bytes.
   @param[in]      InitialValue  Initial value of the CRC.

   @retval UINT32   The checksum.
*/
UINT32
Ext4CalculateChecksum (
  IN CONST EXT4_PARTITION *Partition, IN CONST VOID *Buffer, IN UINTN Length,
  IN UINT32 InitialValue
  )
{
  if(!(Partition->FeaturesRoCompat & EXT4_FEATURE_RO_COMPAT_METADATA_CSUM)) {
    return 0;
  }

  switch(Partition->SuperBlock.s_checksum_type) {
    case EXT4_CHECKSUM_CRC32C:
      // For some reason, EXT4 really likes non-inverted CRC32C checksums, so we stick to that here.
      return ~CalculateCrc32c(Buffer, Length, ~InitialValue);
    default:
      UNREACHABLE ();
  }
}
