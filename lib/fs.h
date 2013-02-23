#ifndef JGFS2_LIB_FS_H
#define JGFS2_LIB_FS_H


#include "jgfs2.h"


struct jgfs2_fs {
	bool init;
	
	struct jgfs2_mkfs_param    mkfs_param;
	struct jgfs2_mount_options mount_opt;
	
	struct jgfs2_superblock *new_sblk;
	
	uint64_t size_byte;
	uint32_t size_blk;
	
	uint32_t blk_size;
	
	uint32_t data_blk_first;
	uint32_t data_blk_count;
	
	struct jgfs2_sect       *vbr;
	struct jgfs2_superblock *sblk;
	struct jgfs2_sect       *boot;
	
	void *fs_bitmap;
	
	struct jgfs2_directory *root_dir;
};


extern struct jgfs2_fs fs;


void *jgfs2_fs_map_sect(uint32_t sect_num, uint32_t sect_count);
void jgfs2_fs_unmap_sect(void *addr, uint32_t sect_num, uint32_t sect_count);
void *jgfs2_fs_map_blk(uint32_t blk_num, uint32_t blk_count);
void jgfs2_fs_unmap_blk(void *addr, uint32_t blk_num, uint32_t blk_count);

bool jgfs2_fs_sblk_check(const struct jgfs2_superblock *sblk);

void jgfs2_fs_init(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt);
void jgfs2_fs_done(void);

void jgfs2_fs_new_pre_init(const struct jgfs2_mkfs_param *param);
void jgfs2_fs_new_post_init(void);


#endif
