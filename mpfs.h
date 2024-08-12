#include <linux/types.h>

#define DEFAULT_BLOCK_SIZE \
	1024 // We will use this for boot block and superblock. Regardless of the FS block size, superblock will always fit withing 1024B.
#define SB_DEFAULT_BLOCK_NUMBER 1
#define INODE_SIZE_BYTES 128
#define MPFS_SUPER_MAGIC 0x4D50

/**
 * mpfs super-block data on disk
 */
struct mpfs_super_block {
	__le16 s_magic;
	__le32 s_disk_size_mb;
	unsigned char s_block_size_log; // 1024 * 2 ^ x
	__le32 s_inode_bm_size; // bytes, not rounded to blocks
	__le32 s_data_bm_size; // bytes, not rounded to blocks
	__le32 s_inode_table_size; // bytes, not rounded to blocks
	__le64 s_num_data_blocks;
};

/*
 * mpfs super-block data in memory
 */
struct minix_sb_info {
	//
};