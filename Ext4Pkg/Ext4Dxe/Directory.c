/**
 * @brief Directory related routines
 *
 * @copyright Copyright (c) 2021 Pedro Falcato
 */

#include "Ext4.h"
#include "Library/BaseLib.h"

EFI_STATUS Ext4RetrieveDirent(EXT4_FILE *File, const CHAR16 *NameUnicode, EXT4_PARTITION *Partition,
			EXT4_DIR_ENTRY *res)
{
    char Name[256];
    if(UnicodeStrToAsciiStrS(NameUnicode, Name, sizeof(Name)) != EFI_SUCCESS)
        return EFI_INVALID_PARAMETER;

    EFI_STATUS st = EFI_NOT_FOUND;
	char *buf = AllocatePages(1);
	if(!buf)
		return EFI_OUT_OF_RESOURCES;

	UINT64 off = 0;

    EXT4_INODE *Inode = File->Inode;
    UINT64 DirInoSize = EXT4_INODE_SIZE(Inode);

	while(off < DirInoSize)
	{
        UINT64 Length = Partition->BlockSize;

		st = Ext4Read(Partition, Inode, buf, off, &Length);

		if(st != EFI_SUCCESS)
		{
			FreePages(buf, 1);
			return st;
		}

		for(char *b = buf; b < buf + Partition->BlockSize; )
		{
			EXT4_DIR_ENTRY *entry = (EXT4_DIR_ENTRY *) b;
			ASSERT(entry->size != 0);

			if(entry->lsbit_namelen == AsciiStrLen(Name) && 
		  	   !CompareMem(entry->name, Name, entry->lsbit_namelen))
			{
				*res = *entry;
				FreePages(buf, 1);
				return EFI_SUCCESS;
			}

			b += entry->size;
		}

		off += Partition->BlockSize;
	}

    FreePages(buf, 1);
	return EFI_NOT_FOUND;
}

EFI_STATUS Ext4OpenFile(EXT4_FILE *Directory, const CHAR16 *Name, EXT4_PARTITION *Partition, UINT64 OpenMode,
                        OUT EXT4_FILE **OutFile)
{
    EXT4_DIR_ENTRY Entry;
    EFI_STATUS st = Ext4RetrieveDirent(Directory, Name, Partition, &Entry);

    if(EFI_ERROR(st))
        return st;
    
    EXT4_FILE *File = AllocateZeroPool(sizeof(EXT4_FILE));
    if(!File)
    {
        FreePool(File);
        return EFI_OUT_OF_RESOURCES;
    }

    File->InodeNum = Entry.inode;

    Ext4SetupFile(File, (EXT4_PARTITION *) Partition);

    st = Ext4ReadInode(Partition, Entry.inode, &File->Inode);

    if(EFI_ERROR(st))
    {
        FreePool(File);
        return st;
    }

    *OutFile = File;

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Ext4OpenVolume(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Partition, EFI_FILE_PROTOCOL **Root)
{
    EXT4_INODE *RootInode;
  
    EFI_STATUS st = Ext4ReadInode((EXT4_PARTITION *) Partition, 2, &RootInode);
    if(EFI_ERROR(st))
    {
        DEBUG((EFI_D_ERROR, "[ext4] Could not open root inode - status %x\n", st));
        return st;
    }

    EXT4_FILE *RootDir = AllocateZeroPool(sizeof(EXT4_FILE));
    if(!RootDir)
    {
        FreePool(RootInode);
        return EFI_OUT_OF_RESOURCES;
    }

    RootDir->Inode = RootInode;
    RootDir->InodeNum = 2;

    Ext4SetupFile(RootDir, (EXT4_PARTITION *) Partition);
    *Root = &RootDir->Protocol;

    return EFI_SUCCESS;
}
