/**
 * @file Extent related routines
 *
  * Copyright (c) 2021 Pedro Falcato All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Ext4.h"

/**
   Caches a range of extents, by allocating pool memory for each extent and adding it to the tree.

   @param[in]      File        Pointer to the open file.
   @param[in]      Extents     Pointer to an array of extents.
   @param[in]      NumberExtents Length of the array.

   @retval none
*/
VOID
Ext4CacheExtents (
  IN EXT4_FILE *File, IN CONST EXT4_EXTENT *Extents, IN UINT16 NumberExtents
  );

/**
   Gets an extent from the extents cache of the file.

   @param[in]      File          Pointer to the open file.
   @param[in]      Block         Block we want to grab.

   @retval EXT4_EXTENT *         Pointer to the extent, or NULL if it was not found.
*/
EXT4_EXTENT *
Ext4GetExtentFromMap (
  IN EXT4_FILE *File, UINT32 Block
  );

/**
   Retrieves the pointer to the top of the extent tree.
   @param[in]      Inode         Pointer to the inode structure.

   @retval EXT4_EXTENT_HEADER    Pointer to an EXT4_EXTENT_HEADER. This pointer is inside
                                 the inode and must not be freed.
*/
STATIC
EXT4_EXTENT_HEADER *
Ext4GetInoExtentHeader (
  IN EXT4_INODE *Inode
  )
{
  return (EXT4_EXTENT_HEADER *)Inode->i_data;
}

/**
   Checks if an extent header is valid.
   @param[in]      Header         Pointer to the EXT4_EXTENT_HEADER structure.

   @retval BOOLEAN    TRUE if valid, FALSE if not.
*/
STATIC
BOOLEAN
Ext4ExtentHeaderValid (
  IN CONST EXT4_EXTENT_HEADER *Header
  )
{
  if(Header->eh_depth > EXT4_EXTENT_TREE_MAX_DEPTH) {
    DEBUG ((EFI_D_ERROR, "[ext4] Invalid extent header depth %u\n", Header->eh_depth));
    return FALSE;
  }

  if(Header->eh_magic != EXT4_EXTENT_HEADER_MAGIC) {
    DEBUG ((EFI_D_ERROR, "[ext4] Invalid extent header magic %x\n", Header->eh_magic));
    return FALSE;
  }

  if(Header->eh_max < Header->eh_entries) {
    DEBUG ((
      EFI_D_ERROR,
      "[ext4] Invalid extent header num entries %u max entries %u\n",
      Header->eh_entries,
      Header->eh_max
      ));
    return FALSE;
  }

  return TRUE;
}

/**
   Performs a binary search for a EXT4_EXTENT_INDEX that corresponds to a 
   logical block in a given extent tree node.

   @param[in]      Header         Pointer to the EXT4_EXTENT_HEADER structure.
   @param[in]      LogicalBlock   Block that will be searched

   @retval EXT4_EXTENT_INDEX*     Pointer to the found EXT4_EXTENT_INDEX.
*/
STATIC
EXT4_EXTENT_INDEX *
Ext4BinsearchExtentIndex (
  IN EXT4_EXTENT_HEADER *Header, IN EXT4_BLOCK_NR LogicalBlock
  )
{
  EXT4_EXTENT_INDEX  *l = ((EXT4_EXTENT_INDEX *)(Header + 1)) + 1;
  EXT4_EXTENT_INDEX  *r = ((EXT4_EXTENT_INDEX *)(Header + 1)) + Header->eh_entries - 1;
  EXT4_EXTENT_INDEX  *m;

  // Perform a mostly-standard binary search on the array
  // This works very nicely because the extents arrays are always sorted.

  while(l <= r) {
    m = l + (r - l) / 2;

    if(LogicalBlock < m->ei_block) {
      r = m - 1;
    } else {
      l = m + 1;
    }
  }

  return l - 1;
}

/**
   Performs a binary search for a EXT4_EXTENT that corresponds to a 
   logical block in a given extent tree node.

   @param[in]      Header         Pointer to the EXT4_EXTENT_HEADER structure.
   @param[in]      LogicalBlock   Block that will be searched

   @retval EXT4_EXTENT_INDEX*     Pointer to the found EXT4_EXTENT_INDEX.
           NULL                   Array is empty.
                                  Note: The caller must check if the logical block
                                  is actually mapped under the given extent.
*/
STATIC
EXT4_EXTENT *
Ext4BinsearchExtentExt (
  IN EXT4_EXTENT_HEADER *Header, IN EXT4_BLOCK_NR LogicalBlock
  )
{
  EXT4_EXTENT  *l = ((EXT4_EXTENT *)(Header + 1)) + 1;
  EXT4_EXTENT  *r = ((EXT4_EXTENT *)(Header + 1)) + Header->eh_entries - 1;
  EXT4_EXTENT  *m;

  // Perform a mostly-standard binary search on the array
  // This works very nicely because the extents arrays are always sorted.

  // Empty array
  if(Header->eh_entries == 0) {
    return NULL;
  }

  while(l <= r) {
    m = l + (r - l) / 2;

    if(LogicalBlock < m->ee_block) {
      r = m - 1;
    } else {
      l = m + 1;
    }
  }

  return l - 1;
}

/**
   Retrieves the leaf block from an EXT4_EXTENT_INDEX.

   @param[in]      Index          Pointer to the EXT4_EXTENT_INDEX structure.

   @retval EXT4_BLOCK_NR          Block number of the leaf node.
*/
STATIC
EXT4_BLOCK_NR
Ext4ExtentIdxLeafBlock (
  IN EXT4_EXTENT_INDEX *Index
  )
{
  return ((UINT64)Index->ei_leaf_hi << 32) | Index->ei_leaf_lo;
}

STATIC UINTN  GetExtentRequests  = 0;
STATIC UINTN  GetExtentCacheHits = 0;

/**
   Retrieves an extent from an EXT4 inode.
   @param[in]      Partition     Pointer to the opened EXT4 partition.
   @param[in]      File          Pointer to the opened file.
   @param[in]      LogicalBlock  Block number which the returned extent must cover.
   @param[out]     Extent        Pointer to the output buffer, where the extent will be copied to.

   @retval EFI_STATUS         Status of the retrieval operation.
           EFI_NO_MAPPING     Block has no mapping.
*/
EFI_STATUS
Ext4GetExtent (
  IN EXT4_PARTITION *Partition, IN EXT4_FILE *File, IN EXT4_BLOCK_NR LogicalBlock, OUT EXT4_EXTENT *Extent
  )
{
  EXT4_INODE  *Inode = File->Inode;

  DEBUG ((EFI_D_INFO, "[ext4] Looking up extent for block %lu\n", LogicalBlock));
  VOID         *Buffer = NULL;
  EXT4_EXTENT  *Ext    = NULL;

  // ext4 does not have support for logical block numbers bigger than UINT32_MAX
  // TODO: Is there UINT32_MAX in Tianocore?
  if (LogicalBlock > (UINT32)- 1) {
    return EFI_NO_MAPPING;
  }

  GetExtentRequests++;
 #if DEBUG_EXTENT_CACHE
    DEBUG ((
      EFI_D_INFO,
      "[ext4] Requests %lu, hits %lu, misses %lu\n",
      GetExtentRequests,
      GetExtentCacheHits,
      GetExtentRequests - GetExtentCacheHits
      ));
 #endif

  // Note: Right now, holes are the single biggest reason for cache misses
  // We should find a way to get (or cache) holes
  if ((Ext = Ext4GetExtentFromMap (File, (UINT32)LogicalBlock)) != NULL) {
    *Extent = *Ext;
    GetExtentCacheHits++;

    return EFI_SUCCESS;
  }

  // Slow path, we'll need to read from disk and (try to) cache those extents.

  EXT4_EXTENT_HEADER  *ExtHeader = Ext4GetInoExtentHeader (Inode);

  if(!Ext4ExtentHeaderValid (ExtHeader)) {
    return EFI_VOLUME_CORRUPTED;
  }

  while(ExtHeader->eh_depth != 0) {
    // While depth != 0, we're traversing the tree itself and not any leaves
    // As such, every entry is an EXT4_EXTENT_INDEX entry
    // Note: Entries after the extent header, either index or actual extent, are always sorted.
    // Therefore, we can use binary search, and it's actually the standard for doing so
    // (see FreeBSD).

    EXT4_EXTENT_INDEX  *Index = Ext4BinsearchExtentIndex (ExtHeader, LogicalBlock);

    if(Buffer == NULL) {
      Buffer = AllocatePool (Partition->BlockSize);
      if(Buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      EFI_STATUS  st;
      if((st = Ext4ReadBlocks (Partition, Buffer, 1, Ext4ExtentIdxLeafBlock (Index))) != EFI_SUCCESS) {
        FreePool (Buffer);
        return st;
      }
    }

    ExtHeader = Buffer;

    if(!Ext4ExtentHeaderValid (ExtHeader)) {
      FreePool (Buffer);
      return EFI_VOLUME_CORRUPTED;
    }
  }

  /* We try to cache every extent under a single leaf, since it's quite likely that we
   * may need to access things sequentially. Furthermore, ext4 block allocation as done
   * by linux (and possibly other systems) is quite fancy and usually it results in a small number of extents.
   * Therefore, we shouldn't have any memory issues.
   */
  Ext4CacheExtents (File, (EXT4_EXTENT *)(ExtHeader + 1), ExtHeader->eh_entries);

  Ext = Ext4BinsearchExtentExt (ExtHeader, LogicalBlock);

  if(!Ext) {
    if(Buffer != NULL) {
      FreePool (Buffer);
    }

    return EFI_NO_MAPPING;
  }

  if(!(LogicalBlock >= Ext->ee_block && Ext->ee_block + Ext->ee_len > LogicalBlock)) {
    // This extent does not cover the block
    if(Buffer != NULL) {
      FreePool (Buffer);
    }

    return EFI_NO_MAPPING;
  }

  *Extent = *Ext;

  if(Buffer != NULL) {
    FreePool (Buffer);
  }

  return EFI_SUCCESS;
}

/**
  Compare two EXT4_EXTENT structs.
  Used in the extent map's ORDERED_COLLECTION.

  @param[in] UserStruct1  Pointer to the first user structure.

  @param[in] UserStruct2  Pointer to the second user structure.

  @retval <0  If UserStruct1 compares less than UserStruct2.

  @retval  0  If UserStruct1 compares equal to UserStruct2.

  @retval >0  If UserStruct1 compares greater than UserStruct2.
**/
STATIC
INTN EFIAPI
Ext4ExtentsMapStructCompare (
  IN CONST VOID *UserStruct1,
  IN CONST VOID *UserStruct2
  )
{
  CONST EXT4_EXTENT  *Extent1 = UserStruct1;
  CONST EXT4_EXTENT  *Extent2 = UserStruct2;

  // TODO: Detect extent overlaps? in case of corruption.

  /* DEBUG((EFI_D_INFO, "[ext4] extent 1 %u extent 2 %u = %ld\n", Extent1->ee_block,
   Extent2->ee_block, Extent1->ee_block - Extent2->ee_block)); */
  return Extent1->ee_block < Extent2->ee_block ? - 1 :
         Extent1->ee_block > Extent2->ee_block ? 1 : 0;
}

/**
  Compare a standalone key against a EXT4_EXTENT containing an embedded key.
  Used in the extent map's ORDERED_COLLECTION.

  @param[in] StandaloneKey  Pointer to the bare key.

  @param[in] UserStruct     Pointer to the user structure with the embedded
                            key.

  @retval <0  If StandaloneKey compares less than UserStruct's key.

  @retval  0  If StandaloneKey compares equal to UserStruct's key.

  @retval >0  If StandaloneKey compares greater than UserStruct's key.
**/
STATIC
INTN EFIAPI
Ext4ExtentsMapKeyCompare (
  IN CONST VOID *StandaloneKey,
  IN CONST VOID *UserStruct
  )
{
  CONST EXT4_EXTENT  *Extent = UserStruct;

  // Note that logical blocks are 32-bits in size so no truncation can happen here
  // with regards to 32-bit architectures.
  UINT32  Block = (UINT32)(UINTN)StandaloneKey;

  // DEBUG((EFI_D_INFO, "[ext4] comparing %u %u\n", Block, Extent->ee_block));
  if(Block >= Extent->ee_block && Block < Extent->ee_block + Extent->ee_len) {
    return 0;
  }

  return Block < Extent->ee_block ? - 1 :
         Block > Extent->ee_block ? 1 : 0;
}

/**
   Initialises the (empty) extents map, that will work as a cache of extents.

   @param[in]      File        Pointer to the open file.

   @retval EFI_STATUS          Result of the operation
*/
EFI_STATUS
Ext4InitExtentsMap (
  IN EXT4_FILE *File
  )
{
  File->ExtentsMap = OrderedCollectionInit (Ext4ExtentsMapStructCompare, Ext4ExtentsMapKeyCompare);
  if (!File->ExtentsMap) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
   Frees the extents map, deleting every extent stored.

   @param[in]      File        Pointer to the open file.

   @retval none
*/
VOID
Ext4FreeExtentsMap (
  IN EXT4_FILE *File
  )
{
  // Keep calling Min(), so we get an arbitrary node we can delete.
  // If Min() returns NULL, it's empty.

  ORDERED_COLLECTION_ENTRY  *MinEntry = NULL;

  while ((MinEntry = OrderedCollectionMin (File->ExtentsMap)) != NULL) {
    EXT4_EXTENT  *Ext;
    OrderedCollectionDelete (File->ExtentsMap, MinEntry, (VOID **)&Ext);
    FreePool (Ext);
  }

  ASSERT (OrderedCollectionIsEmpty (File->ExtentsMap));

  OrderedCollectionUninit (File->ExtentsMap);
  File->ExtentsMap = NULL;
}

/**
   Caches a range of extents, by allocating pool memory for each extent and adding it to the tree.

   @param[in]      File        Pointer to the open file.
   @param[in]      Extents     Pointer to an array of extents.
   @param[in]      NumberExtents Length of the array.

   @retval none
*/
VOID
Ext4CacheExtents (
  IN EXT4_FILE *File, IN CONST EXT4_EXTENT *Extents, IN UINT16 NumberExtents
  )
{
  /* Note that any out of memory condition might mean we don't get to cache a whole leaf of extents
   * in which case, future insertions might fail.
   */

  for(UINT16 i = 0; i < NumberExtents; i++, Extents++) {
    EXT4_EXTENT  *Extent = AllocatePool (sizeof (EXT4_EXTENT));

    if (Extent == NULL) {
      return;
    }

    CopyMem (Extent, Extents, sizeof (EXT4_EXTENT));
    EFI_STATUS  st = OrderedCollectionInsert (File->ExtentsMap, NULL, Extent);

    // EFI_ALREADY_STARTED = already exists in the tree.
    if (EFI_ERROR (st)) {
      FreePool (Extent);

      if(st == EFI_ALREADY_STARTED) {
        continue;
      }

      return;
    }

 #if DEBUG_EXTENT_CACHE
      DEBUG ((
        EFI_D_INFO,
        "[ext4] Cached extent [%lu, %lu]\n",
        Extent->ee_block,
        Extent->ee_block + Extent->ee_len - 1
        ));
 #endif
  }
}

/**
   Gets an extent from the extents cache of the file.

   @param[in]      File          Pointer to the open file.
   @param[in]      Block         Block we want to grab.

   @retval EXT4_EXTENT *         Pointer to the extent, or NULL if it was not found.
*/
EXT4_EXTENT *
Ext4GetExtentFromMap (
  IN EXT4_FILE *File, UINT32 Block
  )
{
  ORDERED_COLLECTION_ENTRY  *Entry = OrderedCollectionFind (File->ExtentsMap, (CONST VOID *)(UINTN)Block);

  if (Entry == NULL) {
    return NULL;
  }

  return OrderedCollectionUserStruct (Entry);
}
