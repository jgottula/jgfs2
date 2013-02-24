#ifndef JGFS2_LIB_NEW_H
#define JGFS2_LIB_NEW_H


#include "jgfs2.h"


void jgfs2_new_init_root_dir(void);

const struct jgfs2_super_block *jgfs2_new_pre(const char *dev_path,
	const struct jgfs2_mkfs_param *param);
void jgfs2_new_post(void);


#endif
