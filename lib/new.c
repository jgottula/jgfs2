/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "new.h"
#include <bsd/string.h>
#include <sys/user.h>
#include <time.h>
#include <uuid/uuid.h>
#include "debug.h"
#include "dev.h"
#include "fs.h"
#include "tree.h"


/* use page-size blocks */
#define JGFS2_DEFAULT_BLK_SIZE (PAGE_SIZE / JGFS2_SECT_SIZE)

/* allocate one inode for every 4 blocks */
#define JGFS2_DEFAULT_INODE_RATIO 4


struct jgfs2_mkfs_param mkfs_param;
struct jgfs2_super_block new_sblk;


void fs_new_init_root_dir(void) {
	/*struct jgfs2_inode *root_inode;
	root_inode = inode_get(0);
	memset(root_inode, 0, sizeof(*root_inode));
	
	root_inode->i_used = 0xffff;
	
	root_inode->i_attr = 0;
	root_inode->i_mode = JGFS2_S_IFDIR | 0755;
	
	root_inode->i_uid = 0;
	root_inode->i_gid = 0;
	
	root_inode->i_size = 0;
	
	root_inode->i_atime = time(NULL);
	root_inode->i_ctime = time(NULL);
	root_inode->i_mtime = time(NULL);
	
	root_inode->i_nlink = 1;
	
	root_inode->i_gen = 0;*/
}

const struct jgfs2_super_block *fs_new(const char *dev_path,
	const struct jgfs2_mkfs_param *param) {
	mkfs_param = *param;
	
	warnx("making new filesystem with label '%s'", mkfs_param.label);
	
	if (mkfs_param.total_sect == 0) {
		warnx("using entire device");
		
		dev_open(dev_path, true, false);
		mkfs_param.total_sect = dev.size_sect;
		dev_close();
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
	
	if (uuid_is_null(mkfs_param.uuid)) {
		uuid_generate_random(mkfs_param.uuid);
	} else {
		memcpy(new_sblk.s_uuid, mkfs_param.uuid, sizeof(new_sblk.s_uuid));
	}
	
	strlcpy(new_sblk.s_label, mkfs_param.label, sizeof(new_sblk.s_label));
	
	return &new_sblk;
}

void fs_new_post(void) {
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
	void *slack = fs_map_sect(JGFS2_BOOT_SECT, fs.sblk->s_boot_sect, true);
	memset(slack, 0, BLK_TO_BYTE(fs.data_blk_first) -
		SECT_TO_BYTE(JGFS2_BOOT_SECT + fs.sblk->s_boot_sect));
	fs_unmap_sect(slack, JGFS2_BOOT_SECT, fs.sblk->s_boot_sect);
	
	TODO("put ext tree at fs end so it grows backwards");
	
	fs.sblk->s_addr_ext_tree  = fs.data_blk_first;
	fs.sblk->s_addr_meta_tree = fs.data_blk_first + 1;
	
	tree_init(fs.sblk->s_addr_ext_tree);
	tree_init(fs.sblk->s_addr_meta_tree);
	
	tree_dump(fs.sblk->s_addr_ext_tree);
	tree_dump(fs.sblk->s_addr_meta_tree);
	
	/* init tree roots */
	/* set used areas as allocated */
	/* add root dir to meta tree */
	
	TODO("ext tree init");
	TODO("meta tree init");
	TODO("root dir and other items");
	
	check_print(check_tree(fs.sblk->s_addr_ext_tree), true);
	check_print(check_tree(fs.sblk->s_addr_meta_tree), true);
}
