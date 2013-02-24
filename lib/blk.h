#ifndef JGFS2_LIB_BLK_H
#define JGFS2_LIB_BLK_H


#include "jgfs2.h"


void jgfs2_blk_bmap(bool alloc, uint32_t blk_num, uint32_t blk_cnt);

bool jgfs2_blk_alloc(uint32_t *blk_num, uint32_t blk_cnt);
bool jgfs2_blk_realloc(uint32_t blk_num, uint32_t blk_cnt, uint32_t new_cnt);
void jgfs2_blk_dealloc(uint32_t blk_num, uint32_t blk_cnt);


#endif
