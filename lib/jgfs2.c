/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "jgfs2.h"
#include "debug.h"
#include "fs.h"
#include "new.h"


static bool lib_init = false;


static void jgfs2_clean_up(void) {
	if (lib_init) {
		warnx("cleaning up");
		
		fs_done();
		
		lib_init = false;
	}
}

void jgfs2_stat(uint32_t *blk_size, uint32_t *blk_total, uint32_t *blk_used) {
	/**blk_size  = fs.blk_size;
	*blk_total = fs.size_blk;
	*blk_used  = blk_bmap_cnt(true);*/
}

static void jgfs2_init_common(void) {
	warnx("version 0x%04x", JGFS2_VER_TOTAL);
	
	atexit(jgfs2_clean_up);
}

void jgfs2_init(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt) {
	jgfs2_init_common();
	
	fs_init(dev_path, mount_opt, NULL);
	
	lib_init = true;
}

void jgfs2_new(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt,
	const struct jgfs2_mkfs_param *param) {
	jgfs2_init_common();
	
	const struct jgfs2_super_block *new_sblk = fs_new(dev_path, param);
	fs_init(dev_path, mount_opt, new_sblk);
	
	lib_init = true;
}

void jgfs2_done(void) {
	jgfs2_clean_up();
}
