/**
 * @brief Inode related routines
 *
 * @copyright Copyright (c) 2021 Pedro Falcato
 */

#include "Ext4.h"

#include <Uefi.h>

EFI_STATUS Ext4Read(EXT4_PARTITION *Partition, EXT4_FILE *File, void *Buffer, UINT64 Offset, IN OUT UINT64 *Length)
{
    //DEBUG((EFI_D_INFO, "Ext4Read[Offset %lu, Length %lu]\n", Offset, *Length));
    EXT4_INODE *Inode = File->Inode;
    UINT64 InodeSize = Ext4InodeSize(Inode);

    if(Offset > InodeSize)
        return EFI_DEVICE_ERROR;
    
    UINT64 CurrentSeek = Offset;
    UINT64 RemainingRead = *Length;
    UINT64 BeenRead = 0;

    if (RemainingRead > InodeSize - Offset)
    {
        RemainingRead = InodeSize - Offset;
    }

    while(RemainingRead != 0)
    {
        UINT64 WasRead = 0;
        EXT4_EXTENT Extent;

        // The algorithm here is to get the extent corresponding to the current block
        // and then read as much as we can from the current extent.

        EFI_STATUS st = Ext4GetExtent(Partition, File, CurrentSeek / Partition->BlockSize, &Extent);

        if(st != EFI_SUCCESS && st != EFI_NO_MAPPING)
            return st;
        
        BOOLEAN HasBackingExtent = st != EFI_NO_MAPPING;

        if(!HasBackingExtent)
        {
            UINT64 HoleOff = CurrentSeek % Partition->BlockSize;
            UINT64 HoleLen = Partition->BlockSize - HoleOff;
            WasRead = HoleLen > RemainingRead ? RemainingRead : HoleLen;
            // TODO: Get the hole size and memset all that
            SetMem(Buffer, WasRead, 0);
        }
        else
        {
            UINT64 ExtentStartBytes = (((UINT64) Extent.ee_start_hi << 32) | Extent.ee_start_lo) * Partition->BlockSize;
            UINT64 ExtentLengthBytes = Extent.ee_len * Partition->BlockSize;
            UINT64 ExtentLogicalBytes = (UINT64) Extent.ee_block * Partition->BlockSize;

            // Our extent offset is the difference between CurrentSeek and ExtentLogicalBytes
            UINT64 ExtentOffset = CurrentSeek - ExtentLogicalBytes;
            UINT64 ExtentMayRead = ExtentLengthBytes - ExtentOffset;
            WasRead = ExtentMayRead > RemainingRead ? RemainingRead : ExtentMayRead;
            
            //DEBUG((EFI_D_INFO, "[ext4] may read %lu, remaining %lu\n", ExtentMayRead, RemainingRead));
            //DEBUG((EFI_D_INFO, "[ext4] Reading block %lu\n", (ExtentStartBytes + ExtentOffset) / Partition->BlockSize));
            st = Ext4ReadDiskIo(Partition, Buffer, WasRead, ExtentStartBytes + ExtentOffset);

            if(EFI_ERROR(st))
            {
                DEBUG((EFI_D_ERROR, "[ext4] Error %x reading [%lu, %lu]\n", st,
                       ExtentStartBytes + ExtentOffset, ExtentStartBytes + ExtentOffset + WasRead - 1));
                return st;
            }
        }

        RemainingRead -= WasRead;
        Buffer = (void *) ((char *) Buffer + WasRead);
        BeenRead += WasRead;
        CurrentSeek += WasRead;
    }

    *Length = BeenRead;

    //DEBUG((EFI_D_INFO, "File length %lu crc %x\n", BeenRead, CalculateCrc32(original, BeenRead)));
    return EFI_SUCCESS;
}

EXT4_INODE *Ext4AllocateInode(EXT4_PARTITION *Partition)
{
    return AllocateZeroPool(Partition->BlockSize);
}

BOOLEAN Ext4FileIsDir(IN const EXT4_FILE *File)
{
    return (File->Inode->i_mode & EXT4_INO_TYPE_DIR) == EXT4_INO_TYPE_DIR;
}

BOOLEAN Ext4FileIsReg(IN const EXT4_FILE *File)
{
    return (File->Inode->i_mode & EXT4_INO_TYPE_REGFILE) == EXT4_INO_TYPE_REGFILE;
}

UINT64 Ext4FilePhysicalSpace(EXT4_FILE *File)
{
    BOOLEAN HugeFile = File->Partition->FeaturesIncompat & EXT4_FEATURE_RO_COMPAT_HUGE_FILE;
    UINT64 Blocks = File->Inode->i_blocks;
    if(HugeFile)
    {
        Blocks |= ((UINT64) File->Inode->i_osd2.data_linux.l_i_blocks_high) << 32;
    
        // If HUGE_FILE is enabled and EXT4_HUGE_FILE_FL is set in the inode's flags, each unit
        // in i_blocks corresponds to an actual filesystem block
        if(File->Inode->i_flags & EXT4_HUGE_FILE_FL)
            return Blocks * File->Partition->BlockSize;
    }

    // Else, each i_blocks unit corresponds to 512 bytes
    return Blocks * 512;
}

void Ext4FileATime(IN EXT4_FILE *File, OUT EFI_TIME *Time)
{
    
}

void Ext4FileMTime(IN EXT4_FILE *File, OUT EFI_TIME *Time){}
void Ext4FileCreateTime(IN EXT4_FILE *File, OUT EFI_TIME *Time){}
