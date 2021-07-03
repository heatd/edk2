/**
 * @brief Directory related routines
 *
 * @copyright Copyright (c) 2021 Pedro Falcato
 */

#include "Ext4.h"
#include "Library/BaseLib.h"
#include "Uefi/UefiBaseType.h"

EFI_STATUS Ext4RetrieveDirent(EXT4_FILE *File, const CHAR16 *Name, EXT4_PARTITION *Partition,
			EXT4_DIR_ENTRY *res)
{
    EFI_STATUS st = EFI_NOT_FOUND;
	CHAR8 *buf = AllocatePages(1);
	if(!buf)
		return EFI_OUT_OF_RESOURCES;

	UINT64 off = 0;

    EXT4_INODE *Inode = File->Inode;
    UINT64 DirInoSize = EXT4_INODE_SIZE(Inode);

	while(off < DirInoSize)
	{
        UINT64 Length = Partition->BlockSize;

		st = Ext4Read(Partition, Inode, buf, off, &Length);

		if (st != EFI_SUCCESS)
		{
			FreePages(buf, 1);
			return st;
		}

		for(CHAR8 *b = buf; b < buf + Partition->BlockSize; )
		{

			EXT4_DIR_ENTRY *entry = (EXT4_DIR_ENTRY *) b;
			ASSERT(entry->size != 0);

            UINTN RemainingBlock = Partition->BlockSize - (b - buf);

            if(entry->lsbit_namelen > RemainingBlock || entry->size > RemainingBlock)
            {
                // Corrupted filesystem
                // TODO: Do the proper ext4 corruption detection thing and dirty the filesystem.
                FreePages(buf, 1);
                return EFI_VOLUME_CORRUPTED;
            }

            // Ignore names bigger than our limit.

            /* Note: I think having a limit is sane because:
               1) It's nicer to work with.
               2) Linux and a number of BSDs also have a filename limit of 255.
             */
            if(entry->lsbit_namelen > EXT4_NAME_MAX)
                continue;

            CHAR8 Utf8NameBuf[EXT4_NAME_MAX + 1];
            CopyMem(Utf8NameBuf, entry->name, entry->lsbit_namelen);

            Utf8NameBuf[entry->lsbit_namelen] = '\0';
            CHAR16 Ucs2FileName[EXT4_NAME_MAX + 1];

            st = AsciiStrToUnicodeStrS(Utf8NameBuf, Ucs2FileName, EXT4_NAME_MAX + 1);

            // This should never fail.
            if (EFI_ERROR(st)) {
                FreePages(buf, 1);
                return st;
            }

			if (entry->lsbit_namelen == StrLen(Name) && 
		  	   !Ext4StrCmpInsensitive(Ucs2FileName, (CHAR16 *) Name))
			{
                UINTN ToCopy = entry->size > sizeof(EXT4_DIR_ENTRY) ? sizeof(EXT4_DIR_ENTRY) : entry->size;
                CopyMem(res, entry, ToCopy);
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
