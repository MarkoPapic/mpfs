#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_DESCRIPTION("mpfs file system module.");
MODULE_AUTHOR("Marko Papic");
MODULE_LICENSE("GPL");

int mpfs_fill_super(struct super_block *s, void *data, int silent)
{
    pr_debug("Mounting mpfs...\n");

    return 0;
}

struct dentry *mpfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
    return mount_bdev(fs_type, flags, dev_name, data, mpfs_fill_super);
}

void mpfs_kill_sb(struct super_block *sb)
{
    pr_debug("Unmounting mpfs...\n");
    // TODO: Cleanup

    kill_block_super(sb);
}

static struct file_system_type fst = {
    .owner = THIS_MODULE,
    .name = "mpfs",
    .mount = mpfs_mount,
    .kill_sb = mpfs_kill_sb};

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