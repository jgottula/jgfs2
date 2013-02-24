#include "debug.h"
#include <inttypes.h>


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
	
	const uint8_t *ptr_b = (const uint8_t *)start;
	const uint8_t *end_b = (const uint8_t *)end;
	
	uint8_t i = 0;
	
	while (ptr_b != end_b) {
		if (i == 0) {
			fprintf(stderr, "%p:  ", ptr_b);
		} else if (i == 8) {
			fputc(' ', stderr);
		}
		
		fprintf(stderr, " %02" PRIu8, *ptr_b);
		
		if (++i == 16) {
			fputc('\n', stderr);
			i = 0;
		}
		
		++ptr_b;
	}
}
