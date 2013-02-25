#include "debug.h"
#include "fs.h"


void dump_mem(const void *ptr, size_t len) {
	warnx("%s: %zu bytes @ %p", __func__, len, ptr);
	
	uintptr_t begin = (uintptr_t)ptr;
	uintptr_t end   = (uintptr_t)((uint8_t *)ptr + len);
	
	/* round down to 16 bytes */
	begin &= ~0xf;
	
	/* round up to 16 bytes */
	if ((end & 0xf) != 0) {
		end &= ~0xf;
		end += 0x10;
	}
	
	/* get log2 of len for display purposes */
	len = (end - begin);
	uint8_t log2_len = 0;
	while (len != 0) {
		len >>= 1;
		++log2_len;
	}
	log2_len = CEIL(log2_len, 4);
	
	/* set len correctly */
	len = (end - begin);
	
	const uint8_t *begin_b = (const uint8_t *)begin;
	const uint8_t *end_b   = (const uint8_t *)end;
	
	const uint8_t *ptr_b = begin_b;
	
	bool skip = false, skip_prev = false;
	while (ptr_b <= end_b) {
		if (ptr_b - begin_b >= 0x10 && end_b - ptr_b >= 0x10) {
			if (memcmp(ptr_b - 0x10, ptr_b, 0x10) == 0) {
				if (!skip_prev) {
					fputs("         *\n", stderr);
					skip = true;
				}
			}
		} else {
			skip = false;
		}
		
		if (!skip) {
			fprintf(stderr, "        %0*" PRIxPTR ": ",
				log2_len, ptr_b - begin_b);
			
			if (ptr_b < end_b) {
				for (const uint8_t *ptr_line = ptr_b;
					ptr_line < ptr_b + 0x10; ++ptr_line) {
					if (ptr_line == ptr_b + 8) {
						fputc(' ', stderr);
					}
					
					fprintf(stderr, " %02" PRIx8, *ptr_line);
				}
			}
			
			fputc('\n', stderr);
		}
		
		skip_prev = skip;
		ptr_b += 0x10;
	}
}

void dump_sect(uint32_t sect_num, uint32_t sect_cnt) {
	void *sect_mem = fs_map_sect(sect_num, sect_cnt);
	dump_mem(sect_mem, SECT_TO_BYTE(sect_cnt));
	fs_unmap_sect(sect_mem, sect_num, sect_cnt);
}

void dump_blk(uint32_t blk_num, uint32_t blk_cnt) {
	void *blk_mem = fs_map_blk(blk_num, blk_cnt);
	dump_mem(blk_mem, BLK_TO_BYTE(blk_cnt));
	fs_unmap_blk(blk_mem, blk_num, blk_cnt);
}
