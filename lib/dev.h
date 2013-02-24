#ifndef JGFS2_LIB_DEV_H
#define JGFS2_LIB_DEV_H


#include "jgfs2.h"


struct jgfs2_dev {
	bool read_only;
	
	const char *path;
	int         fd;
	
	uint64_t size_byte;
	uint32_t size_sect;
};


extern struct jgfs2_dev dev;


void *jgfs2_dev_map_sect(uint32_t sect_num, uint32_t sect_cnt);
void jgfs2_dev_unmap_sect(void *addr, uint32_t sect_num, uint32_t sect_cnt);

void jgfs2_dev_fsync(void);
void jgfs2_dev_msync(void *addr, size_t length);

void jgfs2_dev_open(const char *dev_path, bool read_only);
void jgfs2_dev_close(void);


#endif
