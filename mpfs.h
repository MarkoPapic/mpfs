#include <linux/types.h>

#define MPFS_DEFAULT_BLOCK_SIZE \
	1024 // We will use this for boot block and superblock. Regardless of the FS block size, superblock will always fit withing 1024B.
#define MPFS_SB_DEFAULT_BLOCK_NUMBER 1
#define MPFS_INODE_SIZE_BYTES 64
#define MPFS_SUPER_MAGIC 0x4D50
#define MPFS_DIR_BLOCKS_COUNT 7
#define MPFS_IND_BLOCKS_COUNT 1
#define MPFS_DIND_BLOCKS_COUNT 1
#define MPFS_BLOCKS_COUNT (MPFS_DIR_BLOCKS_COUNT + MPFS_IND_BLOCKS_COUNT + MPFS_DIND_BLOCKS_COUNT)

/**
 * MPFS super-block data on disk.
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

/**
 * MPFS inode data on disk. Struct size on disk is 62 bytes.
 */
struct mpfs_inode {
	__le16 i_mode; // File mode
	__le16 i_uid; // Low 16 bits of Owner Uid
	__le32 i_size; // File size in bytes
	__le32 i_atime; // Access time
	__le32 i_ctime; // Creation time
	__le32 i_mtime; // Modification time
	__le16 i_links_count; // Number of hard links
	__le32 i_blocks_count; // Blocks count
	__le32 i_blocks[MPFS_BLOCKS_COUNT]; // Pointers to blocks
};

/*
 * mpfs super-block data in memory
 */
struct minix_sb_info {
	//
};