/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "debug.h"
#include "fs.h"


struct map_node *map_list = NULL;


void debug_map_push(const void *addr, uint32_t sect_num, uint32_t sect_cnt) {
	fprintf(stderr, "\e[31;1m  MAP %p %08" PRIx32 " %08" PRIx32 "\n\e[0m",
		addr, sect_num, sect_cnt);
	
	if (map_list == NULL) {
		map_list = malloc(sizeof(struct map_node));
		map_list->next = NULL;
	} else {
		struct map_node *new_head = malloc(sizeof(struct map_node));
		new_head->next = map_list;
		map_list = new_head;
	}
	
	map_list->payload.addr = addr;
	
	map_list->payload.sect_num = sect_num;
	map_list->payload.sect_cnt = sect_cnt;
}

void debug_map_pop(const void *addr, uint32_t sect_num, uint32_t sect_cnt) {
	fprintf(stderr, "\e[32;1mUNMAP %p %08" PRIx32 " %08" PRIx32 "\n\e[0m",
		addr, sect_num, sect_cnt);
	
	struct map_node **prev = &map_list, *node = map_list;
	while (node != NULL) {
		if (node->payload.addr == addr &&
			node->payload.sect_num == sect_num &&
			node->payload.sect_cnt == sect_cnt) {
			*prev = node->next;
			free(node);
			
			return;
		}
		
		prev = &node->next;
		node = node->next;
	}
	
	errx("%s: this block doesn't seem to have been mapped", __func__);
}

void debug_map_dump(void) {
	struct map_node *node = map_list;
	while (node != NULL) {
		fprintf(stderr, "\e[33;1m LEAK %p %08" PRIx32 " %08" PRIx32 "\n\e[0m",
			node->payload.addr, node->payload.sect_num, node->payload.sect_cnt);
		
		node = node->next;
	}
}

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
			} else {
				skip = false;
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
