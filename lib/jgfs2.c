#include "jgfs2.h"
#include <signal.h>
#include "debug.h"
#include "fs.h"
#include "new.h"


static void jgfs2_clean_up(void) {
	warnx("cleaning up");
	
	fs_done();
	
	signal(SIGSEGV, SIG_DFL);
}

void jgfs2_stat(uint32_t *blk_size, uint32_t *blk_total, uint32_t *blk_used) {
	/**blk_size  = fs.blk_size;
	*blk_total = fs.size_blk;
	*blk_used  = blk_bmap_cnt(true);*/
}

void jgfs2_new(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt,
	const struct jgfs2_mkfs_param *param) {
	warnx("version 0x%04x", JGFS2_VER_TOTAL);
	
	atexit(jgfs2_clean_up);
	
	const struct jgfs2_super_block *new_sblk = fs_new(dev_path, param);
	fs_init(dev_path, mount_opt, new_sblk);
}

void jgfs2_init(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt) {
	warnx("version 0x%04x", JGFS2_VER_TOTAL);
	
	atexit(jgfs2_clean_up);
	signal(SIGSEGV, sig_handle);
	
	fs_init(dev_path, mount_opt, NULL);
}

void jgfs2_done(void) {
	fs_done();
}
