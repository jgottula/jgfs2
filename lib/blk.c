#include "blk.h"
#include "debug.h"
#include "fs.h"


void jgfs2_blk_bitmap(bool alloc, uint32_t blk_num, uint32_t blk_cnt) {
	if (blk_num + blk_cnt >= fs.size_blk) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") >= %" PRIu32,
			__func__, blk_num, blk_num + blk_cnt, fs.size_blk);
	}
	
	TODO("actually modify the fs bitmap");
}

bool jgfs2_blk_alloc(uint32_t *blk_num, uint32_t blk_cnt) {
	TODO("actually allocate");
	
	return false;
}
