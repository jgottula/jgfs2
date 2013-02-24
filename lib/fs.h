#ifndef JGFS2_LIB_FS_H
#define JGFS2_LIB_FS_H


#include "jgfs2.h"


#define BLK_TO_BYTE(_blk) \
	((_blk) * fs.blk_size)
#define BYTE_TO_BLK(_byte) \
	CEIL((_byte), fs.blk_size)


struct jgfs2_fs {
	bool init;
	
	struct jgfs2_mount_options mount_opt;
	
	uint64_t size_byte;
	uint32_t size_blk;
	
	uint32_t blk_size;
	
	uint32_t data_blk_first;
	uint32_t data_blk_cnt;
	
	struct jgfs2_sect        *vbr;
	struct jgfs2_super_block *sblk;
	struct jgfs2_sect        *boot;
	
	uint64_t free_bmap_size_byte;
	uint32_t free_bmap_size_blk;
	void    *free_bmap;
	
	uint64_t inode_table_size_byte;
	uint32_t inode_table_size_blk;
	void    *inode_table;
	
	struct jgfs2_directory *root_dir;
};


extern struct jgfs2_fs fs;


void *fs_map_sect(uint32_t sect_num, uint32_t sect_cnt);
void fs_unmap_sect(void *addr, uint32_t sect_num, uint32_t sect_cnt);
void *fs_map_blk(uint32_t blk_num, uint32_t blk_cnt);
void fs_unmap_blk(void *addr, uint32_t blk_num, uint32_t blk_cnt);

bool fs_sblk_check(const struct jgfs2_super_block *sblk);

void fs_init(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt,
	const struct jgfs2_super_block *new_sblk);
void fs_done(void);


#endif
