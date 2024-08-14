#include "mpfs.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define WORD_SIZE_BYTES 8
#define BLOCK_SIZE_MULTIPLIER 1024
#define BITS_IN_BYTE 8
#define KB_MULTILPIER 1024
#define DEFAULT_BYTES_PER_INODE 16384

const unsigned char zero_word[] = {
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
}; // A single 64-bit word of zeroes

double log2(double x)
{
	return log10(x) / log10(2);
}

int read_bytes(int fd, void *buf, size_t len)
{
	size_t ret;
	while (len != 0 && (ret = read(fd, buf, len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			perror("error reading the file");
			return -1;
		}
		len -= ret;
		buf += ret;
	}

	return 0;
}

int write_zeroes(int fd, size_t len)
{
	if (len % WORD_SIZE_BYTES != 0) {
		fprintf(stderr, "byte size must be a multiple of %u\n",
			WORD_SIZE_BYTES);
		return -1;
	}

	size_t num_batches = len / WORD_SIZE_BYTES;

	for (int i = 0; i < num_batches; i++) {
		if (write(fd, &zero_word, WORD_SIZE_BYTES) != WORD_SIZE_BYTES) {
			return -1;
		}
	}

	return 0;
}

unsigned long round_to_nearest_multiple(unsigned long n, unsigned long m)
{
	if (n % m == 0) {
		return n;
	} else {
		return (n / m + 1) * m;
	}
}

/*int print_file_bytes(int fd, size_t size, size_t buf_size)
{
	int ret = 0;
	size_t read = 0;
	int read_res;
	char *buf = malloc_char(buf_size);

	while (read < size) {
		read_res = read_file(fd, buf, buf_size);
		if (read_res != 0) {
			ret = 1;
			goto finish;
		}
		print_bytes(buf, buf_size, false);
		read += buf_size;
	}
	printf("\n");

finish:
	free(buf);
	return ret;
} */

int get_device_size(int fd, size_t *size)
{
	//return ioctl(fd, BLKGETSIZE64, size);

	// FIXME: Figure out why you are getting "Inapropriate ioctl for device"
	*size = 104857600;

	return 0;
}

void fill_super_block(size_t block_size, size_t bytes_per_inode,
		      size_t disk_size, struct mpfs_super_block *sb)
{
	sb->s_magic = MPFS_SUPER_MAGIC;
	sb->s_disk_size_mb = disk_size / KB_MULTILPIER / KB_MULTILPIER;
	sb->s_block_size_log = (unsigned char)log2(
		block_size /
		BLOCK_SIZE_MULTIPLIER); // TODO: Ensure this is little endian
	size_t max_num_inodes = disk_size / bytes_per_inode;
	sb->s_inode_table_size =
		max_num_inodes *
		MPFS_INODE_SIZE_BYTES; // TODO: Ensure this is little endian
	size_t inode_table_space =
		round_to_nearest_multiple(sb->s_inode_table_size, block_size);
	sb->s_inode_bm_size = max_num_inodes / BITS_IN_BYTE;
	size_t inode_bm_space =
		round_to_nearest_multiple(sb->s_inode_bm_size, block_size);
	size_t metadata_size =
		MPFS_DEFAULT_BLOCK_SIZE + block_size + inode_bm_space +
		inode_table_space; // We don't have data bitmap size yet, but that is ok.
	size_t num_data_blocks = (disk_size - metadata_size) / block_size;
	sb->s_data_bm_size = num_data_blocks / BITS_IN_BYTE;
	size_t data_bm_space =
		round_to_nearest_multiple(sb->s_data_bm_size, block_size);
	metadata_size += data_bm_space;
	sb->s_num_data_blocks =
		(disk_size - metadata_size) /
		block_size; // Now we can get a more accurate number of data blocks.

	printf("Magic: %u\n", sb->s_magic);
	printf("Disk size: %uMB\n", sb->s_disk_size_mb);
	printf("Block size: %.0fB\n",
	       pow(2, sb->s_block_size_log) * BLOCK_SIZE_MULTIPLIER);
	printf("Max inodes: %lu\n", max_num_inodes);
	printf("Data blocks: %llu\n", sb->s_num_data_blocks);
	printf("Data size: %luMB\n",
	       num_data_blocks * block_size / KB_MULTILPIER / KB_MULTILPIER);
	printf("Data bitmap size: %uKB\n", sb->s_data_bm_size / KB_MULTILPIER);
	printf("Data bitmap size on disk: %luKB\n",
	       data_bm_space / KB_MULTILPIER);
	printf("Inode bitmap size: %uB\n", sb->s_inode_bm_size);
	printf("Inode bitmap size on disk: %luB\n", inode_bm_space);
	printf("Inode table size: %uKB\n",
	       sb->s_inode_table_size / KB_MULTILPIER);
	printf("Inode table size on disk: %luKB\n",
	       inode_table_space / KB_MULTILPIER);
	printf("Total metadata size: %luKB (%.2f%%)\n",
	       metadata_size / KB_MULTILPIER,
	       (float)metadata_size / (float)disk_size * 100);
}

int write_boot_block(int fd)
{
	printf("Writing boot block (%dB)...\n", MPFS_DEFAULT_BLOCK_SIZE);

	return write_zeroes(fd, MPFS_DEFAULT_BLOCK_SIZE);
}

int write_super_block(int fd, struct mpfs_super_block *sb)
{
	size_t size = sizeof(*sb);

	printf("Writing superblock (%luB)...\n", size);

	off_t seek_res = lseek(fd, MPFS_DEFAULT_BLOCK_SIZE, SEEK_SET);
	if (seek_res == -1) {
		perror("seek failed\n");
		return -1;
	}

	if (write(fd, sb, size) != size) {
		return -1;
	}

	return 0;
}

int write_data_bitmap(int fd, struct mpfs_super_block *sb)
{
	size_t block_size =
		pow(2, sb->s_block_size_log) * BLOCK_SIZE_MULTIPLIER;
	size_t data_bm_space =
		round_to_nearest_multiple(sb->s_data_bm_size, block_size);

	printf("Writing data bitmap (%luB)...\n", data_bm_space);

	off_t seek_res = lseek(fd, 2 * MPFS_DEFAULT_BLOCK_SIZE, SEEK_SET);
	if (seek_res == -1) {
		perror("seek failed\n");
		return -1;
	}

	return write_zeroes(fd, data_bm_space);
}

int write_inode_bitmap(int fd, struct mpfs_super_block *sb)
{
	size_t block_size =
		pow(2, sb->s_block_size_log) * BLOCK_SIZE_MULTIPLIER;
	size_t data_bm_space =
		round_to_nearest_multiple(sb->s_data_bm_size, block_size);
	size_t inode_bm_space =
		round_to_nearest_multiple(sb->s_inode_bm_size, block_size);

	printf("Writing inode bitmap (%luB)...\n", inode_bm_space);

	off_t seek_res = lseek(fd, 2 * MPFS_DEFAULT_BLOCK_SIZE + data_bm_space, SEEK_SET);
	if (seek_res == -1) {
		perror("seek failed\n");
		return -1;
	}

	return write_zeroes(fd, inode_bm_space);
}

int write_inode_table(int fd, struct mpfs_super_block *sb)
{
	size_t block_size =
		pow(2, sb->s_block_size_log) * BLOCK_SIZE_MULTIPLIER;
	size_t inode_table_space =
		round_to_nearest_multiple(sb->s_inode_table_size, block_size);
	size_t data_bm_space =
		round_to_nearest_multiple(sb->s_data_bm_size, block_size);
	size_t inode_bm_space =
		round_to_nearest_multiple(sb->s_inode_bm_size, block_size);

	printf("Writing inode table (%luB)...\n", inode_table_space);

	off_t seek_res = lseek(fd, 2 * MPFS_DEFAULT_BLOCK_SIZE + data_bm_space + inode_bm_space, SEEK_SET);
	if (seek_res == -1) {
		perror("seek failed\n");
		return -1;
	}

	return write_zeroes(fd, inode_table_space);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd = -1;
	char *device_path;
	struct mpfs_super_block sb = {};
	size_t disk_size;
	size_t block_size;
	size_t bytes_per_inode;
	size_t inode_bitmap_size;
	size_t data_bitmap_size;

	size_t inode_table_size;
	size_t num_data_blocks;
	size_t metadata_size;

	if (argc < 2) {
		perror("not enough arguments\n");
		ret = -1;
		goto out;
	}

	device_path = argv[1];

	if (argc > 2) {
		block_size = strtol(argv[2], NULL, 10);
		if (block_size <= 0 ||
		    block_size % BLOCK_SIZE_MULTIPLIER != 0) {
			perror("invalid block size\n");
			ret = -1;
			goto out;
		}
	} else {
		block_size = MPFS_DEFAULT_BLOCK_SIZE;
	}

	if (argc > 3) {
		bytes_per_inode = strtol(argv[3], NULL, 10);
		if (bytes_per_inode <= 0 ||
		    bytes_per_inode % WORD_SIZE_BYTES != 0) {
			fprintf(stderr,
				"bytes_per_inode must be a multiple of %u\n",
				WORD_SIZE_BYTES);
			ret = -1;
			goto out;
		}
	} else {
		bytes_per_inode = DEFAULT_BYTES_PER_INODE;
	}

	fd = open(device_path, O_RDWR);
	if (fd == -1) {
		perror("cannot open the device\n");
		ret = -1;
		goto out;
	}

	if (get_device_size(fd, &disk_size) == -1) {
		perror("cannot determine disk size\n");
		ret = -1;
		goto out;
	}

	fill_super_block(block_size, bytes_per_inode, disk_size, &sb);

	if (write_boot_block(fd) != 0) {
		perror("failed to write the boot block\n");
		ret = -1;
		goto out;
	}

	if (write_super_block(fd, &sb) != 0) {
		perror("failed to write the superblock\n");
		ret = -1;
		goto out;
	}

	if (write_data_bitmap(fd, &sb) != 0) {
		perror("failed to write data bitmap\n");
		ret = -1;
		goto out;
	}

	if (write_inode_bitmap(fd, &sb) != 0) {
		perror("failed to write inode bitmap\n");
		ret = -1;
		goto out;
	}

	if (write_inode_table(fd, &sb) != 0) {
		perror("failed to write inode table\n");
		ret = -1;
		goto out;
	}

	goto out;

out:
	if (fd != -1) {
		if (fsync(fd) == -1) {
			perror("fsync failed\n");
			ret = -1;
		}
		close(fd);
	}
	return ret;
}