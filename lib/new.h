#ifndef JGFS2_LIB_NEW_H
#define JGFS2_LIB_NEW_H


#include "jgfs2.h"


void jgfs2_new_init_free_bmap(void);

const struct jgfs2_superblock *jgfs2_new_pre(const char *dev_path,
	const struct jgfs2_mkfs_param *param);
void jgfs2_new_post(void);


#endif
