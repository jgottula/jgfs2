/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */

#ifndef JGFS2_LIB_DEV_H
#define JGFS2_LIB_DEV_H


#include "jgfs2.h"


struct dev {
	bool read_only;
	bool debug_map;
	
	long page_size;
	
	const char *path;
	int         fd;
	
	uint64_t size_byte;
	uint32_t size_sect;
	
	uint32_t map_cnt;
};


extern struct dev dev;


void *dev_map_sect(uint32_t sect_num, uint32_t sect_cnt);
void dev_unmap_sect(void *addr, uint32_t sect_num, uint32_t sect_cnt);

void dev_fsync(void);
void dev_msync(void *addr, size_t length);

void dev_open(const char *dev_path, bool read_only, bool debug_map);
void dev_close(void);


#endif
