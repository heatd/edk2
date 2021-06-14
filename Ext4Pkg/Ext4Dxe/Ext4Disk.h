/**
 * @file Raw filesystem data structures 
 * 
 * @copyright Copyright (c) 2021 Pedro Falcato
 * 
 */

#ifndef _EXT4_DISK_H
#define _EXT4_DISK_H

#include <Uefi.h>

#define EXT4_SUPERBLOCK_OFFSET		1024

#define EXT4_SIGNATURE		0xEF53

#define EXT4_VALID_FS        1
#define EXT4_ERROR_FS        2

#define EXT4_ERRORS_CONTINUE 1
#define EXT4_ERRORS_RO       2
#define EXT4_ERRORS_PANIC    3

#define EXT4_LINUX_ID        0
#define EXT4_GNU_HURD_ID     1
#define EXT4_MASIX_ID        2
#define EXT4_FREEBSD_ID      3
#define EXT4_LITES_ID        4

#define EXT4_GOOD_OLD_REV    0
#define EXT4_DYNAMIC_REV     1

#define EXT4_FEATURE_COMPAT_DIR_PREALLOC          1
#define EXT4_FEATURE_COMPAT_IMAGIC_INODES         2
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL           4
#define EXT4_FEATURE_COMPAT_EXT_ATTR              8
#define EXT4_FEATURE_COMPAT_RESIZE_INO            0x10
#define EXT4_FEATURE_COMPAT_DIR_INDEX             0x20

#define EXT4_FEATURE_INCOMPAT_COMPRESSION         0x1
#define EXT4_FEATURE_INCOMPAT_FILETYPE            0x2
#define EXT4_FEATURE_INCOMPAT_RECOVER             0x4
#define EXT4_FEATURE_INCOMPAT_JOURNAL_DEV         0x8
#define EXT4_FEATURE_INCOMPAT_META_BG             0x10

#define EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER       1
#define EXT4_FEATURE_RO_COMPAT_LARGE_FILE         2
#define EXT4_FEATURE_RO_COMPAT_BTREE_DIR          4

#define EXT4_INO_TYPE_FIFO                       0x1000
#define EXT4_INO_TYPE_CHARDEV                    0x2000
#define EXT4_INO_TYPE_DIR                        0x4000
#define EXT4_INO_TYPE_BLOCKDEV                   0x6000
#define EXT4_INO_TYPE_REGFILE                    0x8000
#define EXT4_INO_TYPE_SYMLINK                    0xA000
#define EXT4_INO_TYPE_UNIX_SOCK                  0xC000

/* Inode flags */
#define EXT4_SECRM_FL                    0x1
#define EXT4_UNRM_FL                     0x2
#define EXT4_COMPR_FL                    0x4
#define EXT4_SYNC_FL                     0x8
#define EXT4_IMMUTABLE_FL                0x10
#define EXT4_APPEND_FL                   0x20
#define EXT4_NODUMP_FL                   0x40
#define EXT4_NOATIME_FL                  0x80
#define EXT4_DIRTY_FL                    0x100
#define EXT4_COMPRBLK_FL                 0x200
#define EXT4_NOCOMPR_FL                  0x400
#define EXT4_ECOMPR_FL                   0x800
#define EXT4_BTREE_FL                    0x1000
#define EXT4_INDEX_FL                    0x2000
#define EXT3_JOURNAL_DATA_FL             0x4000
#define EXT4_RESERVED_FL                 0x80000000

/* File type flags that are stored in the directory entries */
#define EXT4_FT_UNKNOWN         0
#define EXT4_FT_REG_FILE        1
#define EXT4_FT_DIR             2
#define EXT4_FT_CHRDEV          3
#define EXT4_FT_BLKDEV          4
#define EXT4_FT_FIFO            5
#define EXT4_FT_SOCK            6
#define EXT4_FT_SYMLINK         7

typedef struct
{
	UINT32 s_inodes_count;
	UINT32 s_blocks_count;
	UINT32 s_r_blocks_count;
	UINT32 s_free_blocks_count;
	UINT32 s_free_inodes_count;
	UINT32 s_first_data_block;
	UINT32 s_log_block_size;
	UINT32 s_log_frag_size;
	UINT32 s_blocks_per_group;
	UINT32 s_frags_per_group;
	UINT32 s_inodes_per_group;
	UINT32 s_mtime;
	UINT32 s_wtime;
	UINT16 s_mnt_count;
	UINT16 s_max_mnt_count;
	UINT16 s_magic;
	UINT16 s_state;
	UINT16 s_errors;
	UINT16 s_minor_rev_level;
	UINT32 s_lastcheck;
	UINT32 s_check_interval;
	UINT32 s_creator_os;
	UINT32 s_rev_level;
	UINT16 s_def_resuid;
	UINT16 s_def_resgid;


	/* Every field after this comment is revision >= 1 */

	UINT32 s_first_ino;
	UINT16 s_inode_size;
	UINT16 s_block_group_nr;
	UINT32 s_feature_compat;
	UINT32 s_feature_incompat;
	UINT32 s_feature_ro_compat;
	UINT8 s_uuid[16];
	UINT8 s_volume_name[16];
	UINT8 s_last_mounted[64];
	UINT32 s_algo_bitmap;
	UINT8 s_prealloc_blocks;
	UINT8 s_prealloc_dir_blocks;
	UINT16 unused;
	UINT8 s_journal_uuid[16];
	UINT32 s_journal_inum;
	UINT32 s_journal_dev;
	UINT32 s_last_orphan;
	UINT32 s_hash_seed[4];
	UINT8 s_def_hash_version;
	UINT32 s_default_mount_options;
	UINT32 s_first_meta_bg;
} EXT4_SUPERBLOCK;

typedef struct
{
	UINT32 block_usage_addr;
	UINT32 inode_usage_addr;
	UINT32 inode_table_addr;
	UINT16 unallocated_blocks_in_group;
	UINT16 unallocated_inodes_in_group;
	UINT16 used_dirs_count;
} BlockGroupDesc;

#define EXT4_DBLOCKS       12
#define EXT4_IND_BLOCK     12
#define EXT4_DIND_BLOCK    13
#define EXT4_TIND_BLOCK    14
#define EXT4_NR_BLOCKS     15

#define EXT4_GOOD_OLD_INODE_SIZE 128

typedef struct _Ext4Inode
{
	UINT16 i_mode;
	UINT16 i_uid;
	UINT32 i_size_lo;
	UINT32 i_atime;
	UINT32 i_ctime;
	UINT32 i_mtime;
	UINT32 i_dtime;
	UINT16 i_gid;
	UINT16 i_links;
	UINT32 i_blocks;
	UINT32 i_flags;
	UINT32 i_os_spec;
	UINT32 i_data[EXT4_NR_BLOCKS];
	UINT32 i_generation;
	UINT32 i_file_acl;
	UINT32 i_size_hi;
	UINT32 i_faddr;
	UINT32 i_os_spec_val[3];
} EXT4_INODE;

typedef struct
{
	UINT32 inode;
	UINT16 size;
	UINT8 lsbit_namelen;
	UINT8 type_indic;
	char name[255];
} DIR_ENTRY;

#define EXT4_SUPPORTED_COMPAT    0
#define EXT4_SUPPORTED_RO_COMPAT 0
#define EXT4_SUPPORTED_INCOMPAT  0

#endif
