#include "new.h"
#include <bsd/string.h>
#include <inttypes.h>
#include <sys/user.h>
#include <time.h>
#include "blk.h"
#include "debug.h"
#include "dev.h"
#include "fs.h"


/* default to page-size blocks */
#define JGFS2_DEFAULT_BLK_SIZE (PAGE_SIZE / JGFS2_SECT_SIZE)


struct jgfs2_mkfs_param mkfs_param;
struct jgfs2_superblock new_sblk;


void jgfs2_new_init_free_bmap(void) {
	warnx("initializing free space bitmap");
	
	/* free all blocks */
	memset(fs.free_bmap, 0, fs.free_bmap_size_byte);
	
	/* set the pre-data area and the bitmap itself as used */
	jgfs2_blk_bmap_set(true, 0, fs.data_blk_first + fs.free_bmap_size_blk);
}

const struct jgfs2_superblock *jgfs2_new_pre(const char *dev_path,
	const struct jgfs2_mkfs_param *param) {
	mkfs_param = *param;
	
	warnx("making new filesystem with label '%s'", mkfs_param.label);
	
	if (mkfs_param.total_sect == 0) {
		warnx("using entire device");
		
		jgfs2_dev_open(dev_path, true);
		mkfs_param.total_sect = dev.size_sect;
		jgfs2_dev_close();
	}
	
	if (mkfs_param.blk_size == 0) {
		mkfs_param.blk_size = JGFS2_DEFAULT_BLK_SIZE;
		
		warnx("using best block size: %" PRIu32 "-byte blocks",
			SECT_TO_BYTE(mkfs_param.blk_size));
	}
	
	TODO("device size checks");
	/* note that not all size variables have been initialized at this point */
	
	memcpy(new_sblk.s_magic, JGFS2_MAGIC, sizeof(new_sblk.s_magic));
	
	new_sblk.s_ver_major = JGFS2_VER_MAJOR;
	new_sblk.s_ver_minor = JGFS2_VER_MINOR;
	
	new_sblk.s_total_sect = mkfs_param.total_sect;
	new_sblk.s_boot_sect  = mkfs_param.boot_sect;
	
	new_sblk.s_blk_size = mkfs_param.blk_size;
	
	new_sblk.s_ctime = time(NULL);
	new_sblk.s_mtime = 0;
	
	memcpy(new_sblk.s_uuid, mkfs_param.uuid, sizeof(new_sblk.s_uuid));
	
	strlcpy(new_sblk.s_label, mkfs_param.label, sizeof(new_sblk.s_label));
	
	/* locate the free space bitmap in the first available block(s) */
	new_sblk.s_addr_free_bmap = CEIL(JGFS2_BOOT_SECT + new_sblk.s_boot_sect,
		new_sblk.s_blk_size);
	
	return &new_sblk;
}

void jgfs2_new_post(void) {
	if (mkfs_param.zap_vbr) {
		warnx("zapping the volume boot record");
		
		memset(fs.vbr, 0, SECT_TO_BYTE(1));
	}
	
	if (mkfs_param.zap_boot) {
		warnx("zapping the boot area");
		
		memset(fs.boot, 0, SECT_TO_BYTE(fs.sblk->s_boot_sect));
	}
	
	/* always zap the slack space between the end of the boot area and the first
	 * data block */
	void *slack = jgfs2_fs_map_sect(JGFS2_BOOT_SECT, fs.sblk->s_boot_sect);
	memset(slack, 0, BLK_TO_BYTE(fs.data_blk_first) -
		SECT_TO_BYTE(JGFS2_BOOT_SECT + fs.sblk->s_boot_sect));
	jgfs2_fs_unmap_sect(slack, JGFS2_BOOT_SECT, fs.sblk->s_boot_sect);
	
	if (mkfs_param.zap_data) {
		warnx("zapping all data blocks (this may take a long time)");
		
		TODO("zap using larger blocks");
		
		/* zero a block at a time */
		for (uint32_t i = 0; i < fs.data_blk_cnt; ++i) {
			void *blk = jgfs2_fs_map_blk(fs.data_blk_first + i, 1);
			memset(blk, 0, BLK_TO_BYTE(1));
			jgfs2_fs_unmap_blk(blk, fs.data_blk_first + i, 1);
		}
	}
	
	jgfs2_new_init_free_bmap();
}
