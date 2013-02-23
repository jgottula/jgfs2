#include "fs.h"
#include <bsd/string.h>
#include <fcntl.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include "debug.h"
#include "dev.h"


#define BLK_BYTES(_count) \
	(fs.blk_size * _count)

struct jgfs2_fs fs;
static struct jgfs2_fs fs_init = {
	.init = false,
	
	.new_sblk = NULL,
	
	.size_byte = 0,
	.size_blk  = 0,
	
	.blk_size = 0,
	
	.data_blk_first = 0,
	.data_blk_count = 0,
	
	.vbr  = NULL,
	.sblk = NULL,
	.boot = NULL,
};


void *jgfs2_fs_map_sect(uint32_t sect_num, uint32_t sect_count) {
	if (sect_num + sect_count >= fs.sblk->s_total_sect) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") >= %" PRIu32,
			__func__, sect_num, sect_num + sect_count, fs.sblk->s_total_sect);
	}
	
	return jgfs2_dev_map_sect(sect_num, sect_count);
}

void jgfs2_fs_unmap_sect(void *addr, uint32_t sect_num, uint32_t sect_count) {
	if (sect_num + sect_count >= fs.sblk->s_total_sect) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") >= %" PRIu32,
			__func__, sect_num, sect_num + sect_count, fs.sblk->s_total_sect);
	}
	
	jgfs2_dev_unmap_sect(addr, sect_num, sect_count);
}

void *jgfs2_fs_map_blk(uint32_t blk_num, uint32_t blk_count) {
	if (blk_num + blk_count >= fs.size_blk) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") >= %" PRIu32,
			__func__, blk_num, blk_num + blk_count, fs.size_blk);
	}
	
	return jgfs2_dev_map_sect(blk_num * fs.sblk->s_blk_size,
		blk_count * fs.sblk->s_blk_size);
}

void jgfs2_fs_unmap_blk(void *addr, uint32_t blk_num, uint32_t blk_count) {
	if (blk_num + blk_count >= fs.size_blk) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") >= %" PRIu32,
			__func__, blk_num, blk_num + blk_count, fs.size_blk);
	}
	
	jgfs2_dev_unmap_sect(addr, blk_num * fs.sblk->s_blk_size,
		blk_count * fs.sblk->s_blk_size);
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
		warnx("filesystem exceeds device bounds (%" PRIu32 " > %" PRIu32 ")",
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
	
	fs.size_byte = SECT_BYTES(fs.sblk->s_total_sect);
	fs.size_blk  = fs.sblk->s_total_sect / fs.sblk->s_blk_size;
	
	fs.blk_size = SECT_BYTES(fs.sblk->s_blk_size);
	
	fs.data_blk_first = CEIL(JGFS2_BOOT_SECT + fs.sblk->s_boot_sect,
		fs.sblk->s_blk_size);
	fs.data_blk_count = fs.size_blk - fs.data_blk_first;
	
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
	
	if (fs.mkfs_param.blk_size == 0) {
		/* advanced block size choosing algorithm */
		fs.mkfs_param.blk_size = 2;
		
		warnx("using best block size: %" PRIu32 " byte blocks",
			SECT_BYTES(fs.mkfs_param.blk_size));
	}
	
	TODO("device size checks");
	/* note that not all size variables have been initialized at this point */
	
	fs.new_sblk = calloc(1, sizeof(struct jgfs2_superblock));
	
	memcpy(fs.new_sblk->s_magic, JGFS2_MAGIC, sizeof(fs.new_sblk->s_magic));
	
	fs.new_sblk->s_ver_major = JGFS2_VER_MAJOR;
	fs.new_sblk->s_ver_minor = JGFS2_VER_MINOR;
	
	fs.new_sblk->s_total_sect = fs.mkfs_param.total_sect;
	fs.new_sblk->s_boot_sect  = fs.mkfs_param.boot_sect;
	
	fs.new_sblk->s_blk_size = fs.mkfs_param.blk_size;
	
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
	memset(slack, 0, BLK_BYTES(fs.data_blk_first) -
		SECT_BYTES(JGFS2_BOOT_SECT + fs.sblk->s_boot_sect));
	
	if (fs.mkfs_param.zap_data) {
		warnx("zapping all data blocks (this may take a long time)");
		
		/* zero a block at a time */
		for (uint32_t i = 0; i < fs.data_blk_count; ++i) {
			void *blk = jgfs2_fs_map_blk(fs.data_blk_first + i, 1);
			memset(blk, 0, BLK_BYTES(1));
			jgfs2_fs_unmap_blk(blk, fs.data_blk_first + i, 1);
		}
	}
	
	warnx("initializing filesystem structures");
	
	TODO("initialize fs structures");
	TODO("set free space bitmap");
}
