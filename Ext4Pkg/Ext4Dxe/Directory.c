/**
 * @brief Directory related routines
 *
 * Copyright (c) 2021 Pedro Falcato All rights reserved.
 * 
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Ext4.h"

/**
   Retrieves the filename of the directory entry and converts it to UTF-16/UCS-2

   @param[in]      Entry   Pointer to a EXT4_DIR_ENTRY.
   @param[in]      Ucs2FileName   Pointer to an array of CHAR16's, of size EXT4_NAME_MAX + 1.
 
   @retval EFI_SUCCESS   Unicode collation was successfully initialised.
   @retval !EFI_SUCCESS  Failure.
*/
EFI_STATUS Ext4GetUcs2DirentName(EXT4_DIR_ENTRY *Entry, CHAR16 Ucs2FileName[EXT4_NAME_MAX + 1])
{
    CHAR8 Utf8NameBuf[EXT4_NAME_MAX + 1];
    CopyMem(Utf8NameBuf, Entry->name, Entry->lsbit_namelen);

    Utf8NameBuf[Entry->lsbit_namelen] = '\0';

    // TODO: Use RedfishPkg's UTF-8 translation routines.
    // This only handles ASCII which is incorrect
    return AsciiStrToUnicodeStrS(Utf8NameBuf, Ucs2FileName, EXT4_NAME_MAX + 1);
}

/**
   Retrieves a directory entry.

   @param[in]      Directory   Pointer to the opened directory.
   @param[in]      NameUnicode Pointer to the UCS-2 formatted filename.
   @param[in]      Partition   Pointer to the ext4 partition.
   @param[out]     Result      Pointer to the destination directory entry.

   @retval EFI_STATUS          Result of the operation
*/
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

    UINT32 BlockRemainder;

    DivU64x32Remainder(DirInoSize, Partition->BlockSize, &BlockRemainder);
    if(BlockRemainder != 0) {
        // Directory inodes need to have block aligned sizes
        return EFI_VOLUME_CORRUPTED;
    }

	while(off < DirInoSize)
	{
        UINTN Length = Partition->BlockSize;

		st = Ext4Read(Partition, File, buf, off, &Length);

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
            
            // Unused entry
            if(entry->inode == 0)
            {
                b += entry->size;
                continue;
            }

            CHAR16 Ucs2FileName[EXT4_NAME_MAX + 1];

            st = Ext4GetUcs2DirentName(entry, Ucs2FileName);

            /* In theory, this should never fail.
             * In reality, it's quite possible that it can fail, considering filenames in 
             * Linux (and probably other nixes) are just null-terminated bags of bytes, and don't
             * need to form valid ASCII/UTF-8 sequences.
             */
            if (EFI_ERROR(st)) {
                // If we error out, skip this entry
                // I'm not sure if this is correct behaviour, but I don't think there's a precedent here.
                b += entry->size;
                continue;
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

/**
   Opens a file using a directory entry.
 
   @param[in]      Partition   Pointer to the ext4 partition.
   @param[in]      OpenMode    Mode in which the file is supposed to be open.
   @param[out]     OutFile     Pointer to the newly opened file.
   @param[in]      Entry       Directory entry to be used.

   @retval EFI_STATUS          Result of the operation
*/
EFI_STATUS Ext4OpenDirent(EXT4_PARTITION *Partition, UINT64 OpenMode, OUT EXT4_FILE **OutFile,
                          EXT4_DIR_ENTRY *Entry)
{
    EFI_STATUS st;
    EXT4_FILE *File = AllocateZeroPool(sizeof(EXT4_FILE));
    if (!File)
    {
        st = EFI_OUT_OF_RESOURCES;
        goto Error;
    }

    CHAR16 FileName[EXT4_NAME_MAX + 1];

    st = Ext4GetUcs2DirentName(Entry, FileName);

    if (EFI_ERROR(st))
    {
        goto Error;
    }

    File->FileName = AllocateZeroPool(StrSize(FileName));

    if (!File->FileName)
    {
        st = EFI_OUT_OF_RESOURCES;
        goto Error;
    }

    st = Ext4InitExtentsMap(File);

    if (EFI_ERROR(st)) {
        goto Error;
    }

    // This should not fail.
    StrCpyS(File->FileName, EXT4_NAME_MAX + 1, FileName);

    File->InodeNum = Entry->inode;

    Ext4SetupFile(File, (EXT4_PARTITION *) Partition);

    st = Ext4ReadInode(Partition, Entry->inode, &File->Inode);

    if (EFI_ERROR(st))
    {
        goto Error;
    }

    *OutFile = File;

    return EFI_SUCCESS;

Error:
    if (File)
    {
        if (File->FileName) {
            FreePool(File->FileName);
        }

        if (File->ExtentsMap) {
            OrderedCollectionUninit(File->ExtentsMap);
        }

        FreePool(File);
    }

    return st;
}

/**
   Opens a file.

   @param[in]      Directory   Pointer to the opened directory.
   @param[in]      Name        Pointer to the UCS-2 formatted filename.
   @param[in]      Partition   Pointer to the ext4 partition.
   @param[in]      OpenMode    Mode in which the file is supposed to be open.
   @param[out]     OutFile     Pointer to the newly opened file.

   @retval EFI_STATUS          Result of the operation
*/
EFI_STATUS Ext4OpenFile(EXT4_FILE *Directory, const CHAR16 *Name, EXT4_PARTITION *Partition, UINT64 OpenMode,
                        OUT EXT4_FILE **OutFile)
{
    EXT4_DIR_ENTRY Entry;
    EFI_STATUS st = Ext4RetrieveDirent(Directory, Name, Partition, &Entry);

    if (EFI_ERROR(st))
        return st;
    
    // EFI requires us to error out on ".." opens for the root directory
    if (Entry.inode == Directory->InodeNum) {
        return EFI_NOT_FOUND;
    }
    
    return Ext4OpenDirent(Partition, OpenMode, OutFile, &Entry);
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

    // The filename will be "\"(null terminated of course)
    RootDir->FileName = AllocateZeroPool(2);

    if (!RootDir->FileName) {
        FreePool(RootDir);
        FreePool(RootInode);
        return EFI_OUT_OF_RESOURCES;
    }

    RootDir->FileName[0] = L'\\';

    RootDir->Inode = RootInode;
    RootDir->InodeNum = 2;

    if (EFI_ERROR(Ext4InitExtentsMap(RootDir))) {
        FreePool(RootDir->FileName);
        FreePool(RootInode);
        FreePool(RootDir);
        return EFI_OUT_OF_RESOURCES;
    }

    Ext4SetupFile(RootDir, (EXT4_PARTITION *) Partition);
    *Root = &RootDir->Protocol;

    return EFI_SUCCESS;
}

BOOLEAN Ext4ValidDirent(EXT4_DIR_ENTRY *Dirent)
{
    UINTN RequiredSize = Dirent->lsbit_namelen + EXT4_MIN_DIR_ENTRY_LEN;
    if (Dirent->size < RequiredSize)
    {
        DEBUG((EFI_D_ERROR, "[ext4] dirent size %lu too small (compared to %lu)\n", Dirent->size, RequiredSize));
        return FALSE;
    }
    
    // Dirent sizes need to be 4 byte aligned
    if (Dirent->size % 4)
        return FALSE;

    return TRUE;
}

/**
   Reads a directory entry.
 
   @param[in]      Partition   Pointer to the ext4 partition.
   @param[in]      File        Pointer to the open directory.
   @param[out]     Buffer      Pointer to the output buffer.
   @param[in]      Offset      Initial directory position.
   @param[in out] OutLength    Pointer to a UINTN that contains the length of the buffer,
                               and the length of the actual EFI_FILE_INFO after the call. 

   @retval EFI_STATUS          Result of the operation
*/
EFI_STATUS Ext4ReadDir(EXT4_PARTITION *Partition, EXT4_FILE *File, VOID *Buffer, UINT64 Offset, IN OUT UINTN *OutLength)
{
    DEBUG((EFI_D_INFO, "[ext4] Ext4ReadDir offset %lu\n", Offset));
    EXT4_INODE *DirIno = File->Inode;
    EFI_STATUS st = EFI_SUCCESS;

    UINT64 DirInoSize = Ext4InodeSize(DirIno);
    UINTN Len;
    UINT32 BlockRemainder;

    DivU64x32Remainder(DirInoSize, Partition->BlockSize, &BlockRemainder);
    if(BlockRemainder != 0) {
        // Directory inodes need to have block aligned sizes
        return EFI_VOLUME_CORRUPTED;
    }

    EXT4_DIR_ENTRY Entry;

    while(TRUE) {
        // We (try to) read the maximum size of a directory entry at a time
        // Note that we don't need to read any padding that may exist after it.
        Len = sizeof(Entry);
        st = Ext4Read(Partition, File, &Entry, Offset, &Len);

        if (EFI_ERROR(st)) {
            goto Out;
        }

#if 0
        DEBUG((EFI_D_INFO, "[ext4] Length read %lu, offset %lu\n", Len, Offset));
#endif

        if (Len == 0) {
            *OutLength = 0;
            st = EFI_SUCCESS;
            goto Out;
        }

        if (Len < EXT4_MIN_DIR_ENTRY_LEN) {
            st = EFI_VOLUME_CORRUPTED;
            goto Out;
        }

        // Invalid directory entry length
        if (!Ext4ValidDirent(&Entry)) {
            DEBUG((EFI_D_ERROR, "[ext4] Invalid dirent at offset %lu\n", Offset));
            st = EFI_VOLUME_CORRUPTED;
            goto Out;
        }

        DEBUG((EFI_D_INFO, "[ext4] dirent size %lu\n", Entry.size));

        if (Entry.inode == 0) {
            // When inode = 0, it's unused
            Offset += Entry.size;
            continue;
        }

        EXT4_FILE *TempFile = NULL;

        st = Ext4OpenDirent(Partition, EFI_FILE_MODE_READ, &TempFile, &Entry);

        if (EFI_ERROR(st)) {
            goto Out;
        }

        
        // TODO: Is this needed?
        if (!StrCmp(TempFile->FileName, L".") || !StrCmp(TempFile->FileName, L".."))
        {
            Offset += Entry.size;
            Ext4CloseInternal(TempFile);
            continue;
        }

#if 0
        DEBUG((EFI_D_INFO, "[ext4] Listing file %s\n", TempFile->FileName));
#endif

        st = Ext4GetFileInfo(TempFile, Buffer, OutLength);
        if (!EFI_ERROR(st)) {
            File->Position = Offset + Entry.size;
        }

        Ext4CloseInternal(TempFile);

        break;
    }

    st = EFI_SUCCESS;
Out:
    return st;
}
