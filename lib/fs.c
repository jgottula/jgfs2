#include "fs.h"
#include <bsd/string.h>
#include <fcntl.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include "debug.h"
#include "dev.h"


#define BLOCK_BYTES(_count) \
	(fs.block_size * _count)

struct jgfs2_fs fs;
static struct jgfs2_fs fs_init = {
	.init = false,
	
	.new_sblk = NULL,
	
	.size_byte  = 0,
	.size_block = 0,
	
	.block_size = 0,
	
	.data_block_first = 0,
	.data_block_count = 0,
	
	.vbr  = NULL,
	.sblk = NULL,
	.boot = NULL,
};


void *jgfs2_fs_map_sect(uint64_t sect_num, uint64_t sect_count) {
	TODO("hook into jgfs2_dev_map_sect, but do bounds checks with fs size");
	
	return NULL;
}

bool jgfs2_fs_sblk_check(const struct jgfs2_superblock *sblk) {
	if (memcmp(sblk->s_magic, JGFS2_MAGIC, sizeof(sblk->s_magic)) != 0) {
		warnx("not jgfs2 or invalid superblock");
		return false;
	}
	
	if (sblk->s_ver_major != JGFS2_VER_MAJOR ||
		sblk->s_ver_minor != JGFS2_VER_MINOR) {
		warnx("incompatible version (0x%04x)",
			JGFS2_VER_EXPAND(fs.sblk->s_ver_major, fs.sblk->s_ver_minor));
		return false;
	}
	
	if (sblk->s_total_sect > dev.size_sect) {
		warnx("filesystem exceeds device bounds (%" PRIu64 " > %" PRIu64 ")",
			sblk->s_total_sect, dev.size_sect);
		return false;
	}
	
	return true;
}

void jgfs2_fs_init(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt) {
	fs = fs_init;
	fs.mount_opt = *mount_opt;
	
	jgfs2_dev_open(dev_path, fs.mount_opt.read_only);
	
	TODO("device size checks");
	
	/* right now, we are only sure that the vbr and superblock exist */
	fs.size_byte = SECT_BYTES(JGFS2_SBLK_SECT + 1);
	
	fs.vbr  = jgfs2_fs_map_sect(JGFS2_VBR_SECT, 1);
	fs.sblk = jgfs2_fs_map_sect(JGFS2_SBLK_SECT, 1);
	
	if (fs.new_sblk != NULL) {
		TODO("write backup superblocks with new_sblk here?");
		
		memcpy(fs.sblk, fs.new_sblk, sizeof(*fs.sblk));
	}
	
	TODO("verify backup superblocks");
	if (!jgfs2_fs_sblk_check(fs.sblk)) {
		errx(1, "primary superblock validation failed");
	}
	
	fs.size_byte  = SECT_BYTES(fs.sblk->s_total_sect);
	fs.size_block = fs.sblk->s_total_sect / fs.sblk->s_block_size;
	
	fs.block_size = SECT_BYTES(fs.sblk->s_block_size);
	
	fs.data_block_first = CEIL(JGFS2_BOOT_SECT + fs.sblk->s_boot_sect,
		fs.sblk->s_block_size);
	fs.data_block_count = fs.size_block - fs.data_block_first;
	
	fs.boot = jgfs2_fs_map_sect(JGFS2_BOOT_SECT, fs.sblk->s_boot_sect);
	
	if (fs.sblk->s_mtime > time(NULL)) {
		warnx("last mount time is in the future: %s",
			ctime((const time_t *)&fs.sblk->s_mtime));
	}
	
	if (!fs.mount_opt.read_only) {
		fs.sblk->s_mtime = time(NULL);
	}
	
	fs.init = true;
}

void jgfs2_fs_done(void) {
	if (fs.init) {
		TODO("cleanup tasks");
		
		jgfs2_dev_close();
		
		fs.init = false;
	}
}

void jgfs2_fs_new_pre_init(const struct jgfs2_mkfs_param *param) {
	fs.mkfs_param = *param;
	
	warnx("making new filesystem with label '%s'", fs.mkfs_param.label);
	
	if (fs.mkfs_param.total_sect == 0) {
		warnx("using entire device");
		
		int temp_fd;
		if ((temp_fd = open(dev.path, O_RDONLY)) < 0) {
			err(1, "failed to open '%s'", dev.path);
		}
		
		fs.mkfs_param.total_sect =
			lseek(temp_fd, 0, SEEK_END) / JGFS2_SECT_SIZE;
		
		if (close(temp_fd) < 0) {
			err(1, "failed to close '%s'", dev.path);
		}
	}
	
	if (fs.mkfs_param.block_size == 0) {
		/* advanced block size choosing algorithm */
		fs.mkfs_param.block_size = 2;
		
		warnx("using best block size: %" PRIu32 " byte blocks",
			SECT_BYTES(fs.mkfs_param.block_size));
	}
	
	TODO("device size checks");
	/* note that not all size variables have been initialized at this point */
	
	fs.new_sblk = calloc(1, sizeof(struct jgfs2_superblock));
	
	memcpy(fs.new_sblk->s_magic, JGFS2_MAGIC, sizeof(fs.new_sblk->s_magic));
	
	fs.new_sblk->s_ver_major = JGFS2_VER_MAJOR;
	fs.new_sblk->s_ver_minor = JGFS2_VER_MINOR;
	
	fs.new_sblk->s_total_sect = fs.mkfs_param.total_sect;
	fs.new_sblk->s_boot_sect  = fs.mkfs_param.boot_sect;
	
	fs.new_sblk->s_block_size = fs.mkfs_param.block_size;
	
	fs.new_sblk->s_ctime = time(NULL);
	fs.new_sblk->s_mtime = 0;
	
	memcpy(fs.new_sblk->s_uuid, fs.mkfs_param.uuid,
		sizeof(fs.new_sblk->s_uuid));
	
	strlcpy(fs.new_sblk->s_label, fs.mkfs_param.label,
		sizeof(fs.new_sblk->s_label));
}

void jgfs2_fs_new_post_init(void) {
	free(fs.new_sblk);
	fs.new_sblk = NULL;
	
	if (fs.mkfs_param.zap_vbr) {
		warnx("zapping the volume boot record");
		
		memset(fs.vbr, 0, JGFS2_SECT_SIZE);
	}
	
	if (fs.mkfs_param.zap_boot) {
		warnx("zapping the boot area");
		
		memset(fs.boot, 0, SECT_BYTES(fs.sblk->s_boot_sect));
	}
	
	/* always zap the slack space between the end of the boot area and the first
	 * data block */
	void *slack = jgfs2_fs_map_sect(JGFS2_BOOT_SECT, fs.sblk->s_boot_sect);
	memset(slack, 0, BLOCK_BYTES(fs.data_block_first) -
		SECT_BYTES(JGFS2_BOOT_SECT + fs.sblk->s_boot_sect));
	
	if (fs.mkfs_param.zap_data) {
		warnx("zapping all data blocks (this may take a long time)");
		
		for (uint32_t i = 0; i < fs.data_block_count; ++i) {
			/*void *block = jgfs2_fs_map_block(fs.data_block_first + i, 1);
			memset(block, 0, BLOCK_BYTES(1));*/
			
			/* unmap the data block (this is okay since no other maps will exist) */
			// ...
		}
		
		/*void *data = jgfs2_fs_map_block(fs.data_block_first,
			fs.data_block_count);
		memset(data, 0, BLOCK_BYTES(fs.data_block_count));*/
		
		/* unmap the data blocks (this ) */
		//jgfs2_fs_unmap_block(data, fs.data_block_first, fs.data_block_count);
	}
	
	warnx("initializing filesystem structures");
	
	TODO("initialize fs structures");
	TODO("set free space bitmap");
}
