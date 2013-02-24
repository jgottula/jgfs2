#include "jgfs2.h"
#include "blk.h"
#include "debug.h"
#include "fs.h"
#include "new.h"


static void jgfs2_clean_up(void) {
	jgfs2_fs_done();
}

void jgfs2_stat(uint32_t *blk_size, uint32_t *blk_total, uint32_t *blk_used) {
	*blk_size  = fs.blk_size;
	*blk_total = fs.size_blk;
	*blk_used  = jgfs2_blk_bmap_cnt(true);
}

void jgfs2_new(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt,
	const struct jgfs2_mkfs_param *param) {
	warnx("version 0x%04x", JGFS2_VER_TOTAL);
	
	atexit(jgfs2_clean_up);
	
	const struct jgfs2_superblock *new_sblk = jgfs2_new_pre(dev_path, param);
	jgfs2_fs_init(dev_path, mount_opt, new_sblk);
	jgfs2_new_post();
}

void jgfs2_init(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt) {
	warnx("version 0x%04x", JGFS2_VER_TOTAL);
	
	atexit(jgfs2_clean_up);
	
	jgfs2_fs_init(dev_path, mount_opt, NULL);
}

void jgfs2_done(void) {
	jgfs2_fs_done();
}
