#include "mpfs.h"
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_DESCRIPTION("mpfs file system module.");
MODULE_AUTHOR("Marko Papic");
MODULE_LICENSE("GPL");

int mpfs_fill_super(struct super_block *sb, void *data, int silent)
{
	int ret = -EINVAL; // TODO: Set this to 0 once you fully implement it
	struct buffer_head *bh;
	struct mpfs_super_block *ms;

	pr_debug("Mounting mpfs...\n");
	//pr_debug("%s\n", data);
	pr_debug("Initial block size: %luB %lub\n", sb->s_blocksize,
		 sb->s_blocksize_bits);

	if (sb->s_blocksize != MPFS_DEFAULT_BLOCK_SIZE) {
		if (!sb_set_blocksize(sb, MPFS_DEFAULT_BLOCK_SIZE)) {
			pr_err("MPFS: unable to set block size to %u",
			       MPFS_DEFAULT_BLOCK_SIZE);
			ret = -EINVAL;
			goto out;
		}
	}

	pr_debug("Default block size to %luB\n", sb->s_blocksize);

	if (!(bh = sb_bread(sb, MPFS_SB_DEFAULT_BLOCK_NUMBER))) {
		pr_err("MPFS: failed to read the superblock");
		ret = -EIO;
		goto out;
	}

	ms = (struct mpfs_super_block *)bh->b_data;

	if (ms->s_magic != MPFS_SUPER_MAGIC) {
		pr_err("MPFS: unexpected magic number");
		ret = -EINVAL;
		goto out;
	}

	sb->s_max_links = MPFS_MAX_LINKS;
	sb->s_time_min = 0;
	sb->s_time_max = U32_MAX;

	// TODO: Set sb->s_maxbytes

	// TODO: Set sb->s_fs_info
	// TODO: Set sb->s_op
	// TODO: Create and set sb->s_root

	/*pr_debug("Magic: %u\n", ms->s_magic);
	pr_debug("Disk size: %uMB\n", ms->s_disk_size_mb);
	pr_debug("Block size log: %u\n", ms->s_block_size_log);
	pr_debug("Data blocks: %llu\n", ms->s_num_data_blocks);
	pr_debug("Data bitmap size: %uKB\n", ms->s_data_bm_size / 1024);
	pr_debug("Inode bitmap size: %uB\n", ms->s_inode_bm_size);*/

	pr_debug("Final block size %lu\n", sb->s_blocksize);

out:
	// Clean up the allocated memory
	return ret;
}

struct dentry *mpfs_mount(struct file_system_type *fs_type, int flags,
			  const char *dev_name, void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data, mpfs_fill_super);
}

void mpfs_kill_sb(struct super_block *sb)
{
	pr_debug("Unmounting mpfs...\n");
	// TODO: Cleanup

	kill_block_super(sb);
}

static struct file_system_type fst = { .owner = THIS_MODULE,
				       .name = "mpfs",
				       .mount = mpfs_mount,
				       .kill_sb = mpfs_kill_sb,
				       .fs_flags = FS_REQUIRES_DEV };

static int mpfs_init(void)
{
	pr_debug("Registering mpfs...\n");

	// See https://github.com/torvalds/linux/blob/0dd3ee31125508cd67f7e7172247f05b7fd1753a/fs/filesystems.c#L72
	return register_filesystem(&fst);
}

static void mpfs_exit(void)
{
	pr_debug("Removing mpfs...\n");

	unregister_filesystem(&fst);
}

module_init(mpfs_init);
module_exit(mpfs_exit);