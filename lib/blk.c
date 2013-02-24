#include "blk.h"
#include <inttypes.h>
#include "debug.h"
#include "fs.h"


void jgfs2_blk_bmap(bool alloc, uint32_t blk_num, uint32_t blk_cnt) {
	if (blk_num + blk_cnt > fs.size_blk) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") > %" PRIu32,
			__func__, blk_num, blk_num + blk_cnt, fs.size_blk);
	}
	
	uint8_t *bmap_ptr = (uint8_t *)fs.free_bmap + (blk_num / 8);
	
	do {
		if ((blk_num % 8) != 0 || blk_cnt < 8) {
			if (alloc) {
				*bmap_ptr |= (1 << (blk_num % 8));
			} else {
				*bmap_ptr &= ~(1 << (blk_num % 8));
			}
			
			if ((++blk_num % 8) == 0) {
				++bmap_ptr;
			}
			--blk_cnt;
		} else {
			*(bmap_ptr++) = (alloc ? 0xff : 0x00);
			
			blk_num += 8;
			blk_cnt -= 8;
		}
	} while (blk_cnt != 0);
}

bool jgfs2_blk_alloc(uint32_t *blk_num, uint32_t blk_cnt) {
	TODO("actually allocate");
	
	/* find a free span of at least the size requested */
	
	/* align large allocations to 8 blocks */
	if (blk_cnt < 8) {
		
	} else {
		
	}
	
	return false;
}

bool jgfs2_blk_realloc(uint32_t blk_num, uint32_t blk_cnt, uint32_t new_cnt) {
	TODO("actually reallocate");
	
	/* try to resize in place; failing that, search for a free region */
	
	return false;
}

void jgfs2_blk_dealloc(uint32_t blk_num, uint32_t blk_cnt) {
	TODO("actually deallocate");
	
	/* check that these blocks really are allocated, and explode if not */
}
