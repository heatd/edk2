/**
 * @brief Extent related routines
 *
 * @copyright Copyright (c) 2021 Pedro Falcato
 */

#include "Ext4.h"
#include "Uefi/UefiBaseType.h"

#include <Uefi.h>

static EXT4_EXTENT_HEADER *Ext4GetInoExtentHeader(EXT4_INODE *Inode)
{
    return (EXT4_EXTENT_HEADER *) Inode->i_data;
}

static BOOLEAN Ext4ExtentHeaderValid(EXT4_EXTENT_HEADER *Header)
{
    if(Header->eh_depth > EXT4_EXTENT_TREE_MAX_DEPTH)
    {
        DEBUG((EFI_D_ERROR, "[ext4] Invalid extent header depth %u\n", Header->eh_depth));
        return FALSE;
    }
    
    if(Header->eh_magic != EXT4_EXTENT_HEADER_MAGIC)
    {
        DEBUG((EFI_D_ERROR, "[ext4] Invalid extent header magic %x\n", Header->eh_magic));
        return FALSE;
    }

    if(Header->eh_max < Header->eh_entries)
    {
        DEBUG((EFI_D_ERROR, "[ext4] Invalid extent header num entries %u max entries %u\n",
                            Header->eh_entries, Header->eh_max));
        return FALSE;
    }

    return TRUE;
}

static EXT4_EXTENT_INDEX *Ext4BinsearchExtentIndex(EXT4_EXTENT_HEADER *Header, EXT4_BLOCK_NR LogicalBlock)
{
    EXT4_EXTENT_INDEX *l = ((EXT4_EXTENT_INDEX*) (Header + 1)) + 1;
    EXT4_EXTENT_INDEX *r = ((EXT4_EXTENT_INDEX*) (Header + 1)) + Header->eh_entries - 1;
    EXT4_EXTENT_INDEX *m;

    // Perform a mostly-standard binary search on the array
    // This works very nicely because the extents arrays are always sorted.

    while(l <= r)
    {
        m = l + (r - l) / 2;

        if(LogicalBlock < m->ei_block)
            r = m - 1;
        else
            l = m + 1;
    }

    return l - 1;
}

static EXT4_EXTENT *Ext4BinsearchExtentExt(EXT4_EXTENT_HEADER *Header, EXT4_BLOCK_NR LogicalBlock)
{
    EXT4_EXTENT *l = ((EXT4_EXTENT*) (Header + 1)) + 1;
    EXT4_EXTENT *r = ((EXT4_EXTENT*) (Header + 1)) + Header->eh_entries - 1;
    EXT4_EXTENT *m;

    // Perform a mostly-standard binary search on the array
    // This works very nicely because the extents arrays are always sorted.

    // Empty array
    if(Header->eh_entries == 0)
        return NULL;

    while(l <= r)
    {
        m = l + (r - l) / 2;

        if(LogicalBlock < m->ee_block)
            r = m - 1;
        else
            l = m + 1;
    }

    return l - 1;
}

EXT4_BLOCK_NR Ext4ExtentIdxLeafBlock(EXT4_EXTENT_INDEX *Index)
{
    return ((UINT64) Index->ei_leaf_hi << 32) | Index->ei_leaf_lo;
}

EFI_STATUS Ext4GetExtent(EXT4_PARTITION *Partition, EXT4_INODE *Inode, EXT4_BLOCK_NR LogicalBlock, OUT EXT4_EXTENT *Extent)
{
    DEBUG((EFI_D_INFO, "[ext4] Looking up extent for block %lu\n", LogicalBlock));
    void *Buffer = NULL;
    EXT4_EXTENT_HEADER *ExtHeader = Ext4GetInoExtentHeader(Inode);

    if(!Ext4ExtentHeaderValid(ExtHeader))
        return EFI_VOLUME_CORRUPTED;
    
    while(ExtHeader->eh_depth != 0)
    {
        // While depth != 0, we're traversing the tree itself and not any leaves
        // As such, every entry is an EXT4_EXTENT_INDEX entry
        // Note: Entries after extent header, index or actual extent, are always sorted.
        // Therefore, we can use binary search, and it's actually the standard for doing so
        // (see FreeBSD)

        EXT4_EXTENT_INDEX *Index = Ext4BinsearchExtentIndex(ExtHeader, LogicalBlock);

        if(!Buffer)
        {
            Buffer = AllocatePool(Partition->BlockSize);
            if(!Buffer)
                return EFI_OUT_OF_RESOURCES;
            
            EFI_STATUS st;
            if((st = Ext4ReadBlocks(Partition, Buffer, 1, Ext4ExtentIdxLeafBlock(Index))) != EFI_SUCCESS)
            {
                FreePool(Buffer);
                return st;
            }
        }

        ExtHeader = Buffer;

        if(!Ext4ExtentHeaderValid(ExtHeader))
        {
            FreePool(Buffer);
            return EFI_VOLUME_CORRUPTED;
        }
    }

    EXT4_EXTENT *ext = Ext4BinsearchExtentExt(ExtHeader, LogicalBlock);

    if(!ext)
    {
        if(Buffer) FreePool(Buffer);
        return EFI_NO_MAPPING;
    }

    if(!(LogicalBlock >= ext->ee_block && ext->ee_block + ext->ee_len > LogicalBlock))
    {
        // This extent does not cover the block
        if(Buffer) FreePool(Buffer);
        return EFI_NO_MAPPING;
    }

    *Extent = *ext;

    if(Buffer) FreePool(Buffer);

    return EFI_SUCCESS;
}
