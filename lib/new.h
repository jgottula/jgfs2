#ifndef JGFS2_LIB_NEW_H
#define JGFS2_LIB_NEW_H


#include "jgfs2.h"


void jgfs2_new_init_free_bmap_pre(void);
void jgfs2_new_init_free_bmap_post(void);
void jgfs2_new_init_inode_table(void);
void jgfs2_new_init_root_dir(void);

const struct jgfs2_superblock *jgfs2_new_pre(const char *dev_path,
	const struct jgfs2_mkfs_param *param);
void jgfs2_new_post(void);


#endif
