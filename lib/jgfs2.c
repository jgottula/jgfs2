#include "jgfs2.h"
#include "debug.h"
#include "fs.h"


static void jgfs2_clean_up(void) {
	jgfs2_fs_done();
}

void jgfs2_init(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt) {
	warnx("version 0x%04x", JGFS2_VER_TOTAL);
	
	atexit(jgfs2_clean_up);
	
	jgfs2_fs_init(dev_path, mount_opt);
}

void jgfs2_new(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt,
	const struct jgfs2_mkfs_param *param) {
	warnx("version 0x%04x", JGFS2_VER_TOTAL);
	
	atexit(jgfs2_clean_up);
	
	jgfs2_fs_new_pre_init(param);
	jgfs2_fs_init(dev_path, mount_opt);
	jgfs2_fs_new_post_init();
}
