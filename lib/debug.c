/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "debug.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include "fs.h"


struct map_node *map_list = NULL;


uint8_t log_u32(uint8_t base, uint32_t n) {
	uint8_t result = 0;
	
	if (n == 0) {
		return 1;
	}
	
	do {
		n /= base;
		++result;
	} while (n != 0);
	
	return result;
}

int fprintf_col(FILE *stream, int col, const char *format, ...) {
	static int cols = 0;
	if (cols == 0) {
		/* fall back to 80 columns if this fails */
		struct winsize ws;
		if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) < 0 ||
			(cols = ws.ws_col) == 0) {
			cols = 80;
		}
	}
	
	/* interpret negative columns as right-aligned */
	if (col < 0) {
		col = cols + col;
	}
	
	int result = fprintf(stream, "\r\e[%dC", col);
	
	va_list ap;
	va_start(ap, format);
	result += vfprintf(stream, format, ap);
	va_end(ap);
	
	return result;
}

void debug_map_push(const void *addr, uint32_t sect_num, uint32_t sect_cnt) {
	fprintf(stderr, "\e[31;1m  MAP %p %08" PRIx32 " %08" PRIx32 "\n\e[0m",
		addr, sect_num, sect_cnt);
	
	struct map_node *new_head = malloc(sizeof(struct map_node));
	new_head->next = map_list;
	map_list = new_head;
	
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

void dump_mem(const void *addr, size_t len) {
	const uint8_t *begin = addr;
	const uint8_t *end   = begin + len;
	
	uint32_t log16_len = log_u32(16, len);
	
	const uint8_t *ptr = begin;
	bool skip = false, skip_prev = false;
	while (ptr <= end) {
		if (ptr - begin >= 0x10 && end - ptr >= 0x10) {
			if (memcmp(ptr - 0x10, ptr, 0x10) == 0) {
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
			fprintf(stderr, "        %0*" PRIxPTR ": ", log16_len, ptr - begin);
			
			if (ptr < end) {
				for (const uint8_t *ptr_line = ptr;
					ptr_line < ptr + 0x10 && ptr_line < end; ++ptr_line) {
					if (ptr_line == ptr + 8) {
						fputc(' ', stderr);
					}
					
					fprintf(stderr, " %02" PRIx8, *ptr_line);
				}
			}
			
			fputc('\n', stderr);
		}
		
		skip_prev = skip;
		ptr += 0x10;
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
