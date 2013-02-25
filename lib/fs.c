#include "fs.h"
#include <time.h>
#include "debug.h"
#include "dev.h"
#include "new.h"


struct fs fs;
static struct fs fs_null = {
	.init = false,
	
	.size_byte = 0,
	.size_blk  = 0,
	
	.blk_size = 0,
	
	.data_blk_first = 0,
	.data_blk_cnt   = 0,
	
	.vbr  = NULL,
	.sblk = NULL,
	.boot = NULL,
	
	.free_bmap_size_byte = 0,
	.free_bmap_size_blk  = 0,
	.free_bmap           = NULL,
	
	.root_dir = NULL,
};


void *fs_map_sect(uint32_t sect_num, uint32_t sect_cnt) {
	if (sect_num + sect_cnt > fs.sblk->s_total_sect) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") > %" PRIu32,
			__func__, sect_num, sect_num + sect_cnt, fs.sblk->s_total_sect);
	}
	
	return dev_map_sect(sect_num, sect_cnt);
}

void fs_unmap_sect(void *addr, uint32_t sect_num, uint32_t sect_cnt) {
	if (sect_num + sect_cnt > fs.sblk->s_total_sect) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") > %" PRIu32,
			__func__, sect_num, sect_num + sect_cnt, fs.sblk->s_total_sect);
	}
	
	dev_unmap_sect(addr, sect_num, sect_cnt);
}

void *fs_map_blk(uint32_t blk_num, uint32_t blk_cnt) {
	if (blk_num + blk_cnt > fs.size_blk) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") > %" PRIu32,
			__func__, blk_num, blk_num + blk_cnt, fs.size_blk);
	}
	
	return dev_map_sect(blk_num * fs.sblk->s_blk_size,
		blk_cnt * fs.sblk->s_blk_size);
}

void fs_unmap_blk(void *addr, uint32_t blk_num, uint32_t blk_cnt) {
	if (blk_num + blk_cnt > fs.size_blk) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") > %" PRIu32,
			__func__, blk_num, blk_num + blk_cnt, fs.size_blk);
	}
	
	dev_unmap_sect(addr, blk_num * fs.sblk->s_blk_size,
		blk_cnt * fs.sblk->s_blk_size);
}

bool fs_sblk_check(const struct jgfs2_super_block *sblk) {
	if (memcmp(sblk->s_magic, JGFS2_MAGIC, sizeof(sblk->s_magic)) != 0) {
		warnx("not jgfs2 or invalid super block");
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

void fs_init(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt,
	const struct jgfs2_super_block *new_sblk) {
	fs = fs_null;
	fs.mount_opt = *mount_opt;
	
	dev_open(dev_path, fs.mount_opt.read_only);
	
	TODO("device size checks");
	
	fs.vbr  = dev_map_sect(JGFS2_VBR_SECT, 1);
	fs.sblk = dev_map_sect(JGFS2_SBLK_SECT, 1);
	
	if (new_sblk != NULL) {
		TODO("write backup super blocks with new_sblk here?");
		
		memcpy(fs.sblk, new_sblk, sizeof(*fs.sblk));
	}
	
	TODO("verify backup super blocks");
	if (!fs_sblk_check(fs.sblk)) {
		errx(1, "primary super block validation failed");
	}
	
	fs.size_byte = SECT_TO_BYTE(fs.sblk->s_total_sect);
	fs.size_blk  = fs.sblk->s_total_sect / fs.sblk->s_blk_size;
	
	fs.blk_size = SECT_TO_BYTE(fs.sblk->s_blk_size);
	
	fs.data_blk_first = CEIL(JGFS2_BOOT_SECT + fs.sblk->s_boot_sect,
		fs.sblk->s_blk_size);
	fs.data_blk_cnt   = fs.size_blk - fs.data_blk_first;
	
	fs.boot = fs_map_sect(JGFS2_BOOT_SECT, fs.sblk->s_boot_sect);
	
	if (new_sblk != NULL) {
		fs_new_post();
	}
	
	if (fs.sblk->s_mtime > time(NULL)) {
		warnx("last mount time is in the future: %s",
			ctime((const time_t *)&fs.sblk->s_mtime));
	}
	
	if (!fs.mount_opt.read_only) {
		fs.sblk->s_mtime = time(NULL);
	}
	
	fs.init = true;
}

void fs_done(void) {
	if (fs.init) {
		fs_unmap_sect(fs.boot, JGFS2_BOOT_SECT, fs.sblk->s_boot_sect);
		
		dev_unmap_sect(fs.vbr, JGFS2_VBR_SECT, 1);
		dev_unmap_sect(fs.sblk, JGFS2_SBLK_SECT, 1);
		
		dev_close();
		
		fs.init = false;
	}
}
