#include "debug.h"


void dump_mem(const void *ptr, size_t len) {
	uintptr_t start = (uintptr_t)ptr;
	uintptr_t end   = (uintptr_t)((uint8_t *)ptr + len);
	
	/* round down to 16 bytes */
	start &= ~0xf;
	
	/* round up to 16 bytes */
	if ((end & 0xf) != 0) {
		end &= ~0xf;
		end += 0x10;
	}
	
	size_t real_len = end - start;
	
	const uint8_t *ptr_b = (const uint8_t *)start;
	const uint8_t *end_b = (const uint8_t *)end;
	
	uint32_t i = 0;
	bool skip = false;
	while (ptr_b != end_b) {
		if (i % 16 == 0) {
			if (i >= 0x10 && real_len >= 0x10) {
				if (memcmp(ptr_b - 0x10, ptr_b, 0x10) == 0) {
					if (!skip) {
						fputs("*\n", stderr);
						skip = true;
					}
					
					ptr_b    += 0x10;
					real_len -= 0x10;
					
					continue;
				} else {
					skip = false;
				}
			} else {
				skip = false;
			}
		}
		
		if (i % 16 == 0) {
			fprintf(stderr, "%p:  ", ptr_b);
		} else if (i % 16 == 8) {
			fputc(' ', stderr);
		}
		
		fprintf(stderr, " %02" PRIu8, *ptr_b);
		
		if (++i % 16 == 0) {
			fputc('\n', stderr);
		}
		
		++ptr_b;
		--real_len;
	}
}
