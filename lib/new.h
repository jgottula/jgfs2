/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#ifndef JGFS2_LIB_NEW_H
#define JGFS2_LIB_NEW_H


#include "jgfs2.h"


void fs_new_init_root_dir(void);

const struct jgfs2_super_block *fs_new(const char *dev_path,
	const struct jgfs2_mkfs_param *param);
void fs_new_post(void);


#endif
