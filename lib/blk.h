#ifndef JGFS2_LIB_BLK_H
#define JGFS2_LIB_BLK_H


#include "jgfs2.h"


uint32_t jgfs2_blk_bmap_cnt(bool alloc);
bool jgfs2_blk_bmap_isfree(uint32_t blk_num, uint32_t blk_cnt);
void jgfs2_blk_bmap_set(bool alloc, uint32_t blk_num, uint32_t blk_cnt);

bool jgfs2_blk_alloc(uint32_t *blk_num, uint32_t blk_cnt);
bool jgfs2_blk_realloc(uint32_t blk_num, uint32_t blk_cnt, uint32_t new_cnt);
void jgfs2_blk_dealloc(uint32_t blk_num, uint32_t blk_cnt);


#endif
