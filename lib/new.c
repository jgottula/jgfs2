#include "new.h"
#include <bsd/string.h>
#include <inttypes.h>
#include <sys/user.h>
#include <time.h>
#include "blk.h"
#include "debug.h"
#include "dev.h"
#include "fs.h"
#include "inode.h"


/* use page-size blocks */
#define JGFS2_DEFAULT_BLK_SIZE (PAGE_SIZE / JGFS2_SECT_SIZE)

/* allocate one inode for every 4 blocks */
#define JGFS2_DEFAULT_INODE_RATIO 4


struct jgfs2_mkfs_param mkfs_param;
struct jgfs2_superblock new_sblk;


void jgfs2_new_init_free_bmap_pre(void) {
	warnx("initializing free space bitmap");
	
	/* locate the free space bitmap in the first available block(s) */
	fs.sblk->s_addr_free_bmap = CEIL(JGFS2_BOOT_SECT + new_sblk.s_boot_sect,
		new_sblk.s_blk_size);
}

void jgfs2_new_init_free_bmap_post(void) {
	/* free all blocks */
	memset(fs.free_bmap, 0, fs.free_bmap_size_byte);
	
	/* set the pre-data area and the bitmap itself as used */
	jgfs2_blk_bmap_set(true, 0, fs.data_blk_first + fs.free_bmap_size_blk);
}

void jgfs2_new_init_inode_table(void) {
	uint32_t inode_cnt  = CEIL(fs.size_blk, JGFS2_DEFAULT_INODE_RATIO);
	uint64_t inode_byte = inode_cnt * sizeof(struct jgfs2_inode);
	uint32_t inode_blk  = CEIL(inode_byte, fs.size_blk);
	
	fs.sblk->s_ext_inode_table.e_len = inode_byte;
	if (!jgfs2_blk_alloc(&fs.sblk->s_ext_inode_table.e_addr, inode_blk)) {
		errx(1, "could not allocate an extent for the inode table");
	}
}

void jgfs2_new_init_root_dir(void) {
	struct jgfs2_inode *root_inode;
	root_inode = jgfs2_inode_get(0);
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
	
	root_inode->i_gen = 0;
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
}
