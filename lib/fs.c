#include "fs.h"
#include <inttypes.h>
#include <time.h>
#include "debug.h"
#include "dev.h"


struct jgfs2_fs fs;
static struct jgfs2_fs fs_init = {
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


void *jgfs2_fs_map_sect(uint32_t sect_num, uint32_t sect_cnt) {
	if (sect_num + sect_cnt > fs.sblk->s_total_sect) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") > %" PRIu32,
			__func__, sect_num, sect_num + sect_cnt, fs.sblk->s_total_sect);
	}
	
	return jgfs2_dev_map_sect(sect_num, sect_cnt);
}

void jgfs2_fs_unmap_sect(void *addr, uint32_t sect_num, uint32_t sect_cnt) {
	if (sect_num + sect_cnt > fs.sblk->s_total_sect) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") > %" PRIu32,
			__func__, sect_num, sect_num + sect_cnt, fs.sblk->s_total_sect);
	}
	
	jgfs2_dev_unmap_sect(addr, sect_num, sect_cnt);
}

void *jgfs2_fs_map_blk(uint32_t blk_num, uint32_t blk_cnt) {
	if (blk_num + blk_cnt > fs.size_blk) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") > %" PRIu32,
			__func__, blk_num, blk_num + blk_cnt, fs.size_blk);
	}
	
	return jgfs2_dev_map_sect(blk_num * fs.sblk->s_blk_size,
		blk_cnt * fs.sblk->s_blk_size);
}

void jgfs2_fs_unmap_blk(void *addr, uint32_t blk_num, uint32_t blk_cnt) {
	if (blk_num + blk_cnt > fs.size_blk) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") > %" PRIu32,
			__func__, blk_num, blk_num + blk_cnt, fs.size_blk);
	}
	
	jgfs2_dev_unmap_sect(addr, blk_num * fs.sblk->s_blk_size,
		blk_cnt * fs.sblk->s_blk_size);
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
	const struct jgfs2_mount_options *mount_opt,
	const struct jgfs2_superblock *new_sblk) {
	fs = fs_init;
	fs.mount_opt = *mount_opt;
	
	jgfs2_dev_open(dev_path, fs.mount_opt.read_only);
	
	TODO("device size checks");
	
	fs.vbr  = jgfs2_dev_map_sect(JGFS2_VBR_SECT, 1);
	fs.sblk = jgfs2_dev_map_sect(JGFS2_SBLK_SECT, 1);
	
	if (new_sblk != NULL) {
		TODO("write backup superblocks with new_sblk here?");
		
		memcpy(fs.sblk, new_sblk, sizeof(*fs.sblk));
	}
	
	TODO("verify backup superblocks");
	if (!jgfs2_fs_sblk_check(fs.sblk)) {
		errx(1, "primary superblock validation failed");
	}
	
	fs.size_byte = SECT_TO_BYTE(fs.sblk->s_total_sect);
	fs.size_blk  = fs.sblk->s_total_sect / fs.sblk->s_blk_size;
	
	fs.blk_size = SECT_TO_BYTE(fs.sblk->s_blk_size);
	
	fs.data_blk_first = CEIL(JGFS2_BOOT_SECT + fs.sblk->s_boot_sect,
		fs.sblk->s_blk_size);
	fs.data_blk_cnt   = fs.size_blk - fs.data_blk_first;
	
	fs.boot = jgfs2_fs_map_sect(JGFS2_BOOT_SECT, fs.sblk->s_boot_sect);
	
	fs.free_bmap_size_byte = CEIL(fs.size_blk, 8);
	fs.free_bmap_size_blk  = BYTE_TO_BLK(fs.free_bmap_size_byte);
	fs.free_bmap           = jgfs2_fs_map_blk(fs.sblk->s_addr_free_bmap,
		fs.free_bmap_size_blk);
	
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
		jgfs2_fs_unmap_blk(fs.free_bmap, fs.sblk->s_addr_free_bmap,
			fs.free_bmap_size_blk);
		
		jgfs2_fs_unmap_sect(fs.boot, JGFS2_BOOT_SECT, fs.sblk->s_boot_sect);
		
		jgfs2_dev_unmap_sect(fs.vbr, JGFS2_VBR_SECT, 1);
		jgfs2_dev_unmap_sect(fs.sblk, JGFS2_SBLK_SECT, 1);
		
		jgfs2_dev_close();
		
		fs.init = false;
	}
}
