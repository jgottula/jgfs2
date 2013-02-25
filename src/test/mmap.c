#include "../../lib/jgfs2.h"
#include "../../lib/debug.h"
#include "../../lib/dev.h"


void test_mmap(void) {
	fprintf(stderr, "%s", __func__);
	
	dev_open("/dev/loop0p1", false);
	
	void *ptr1 = dev_map_sect(0, 1);
	void *ptr2 = dev_map_sect(0, 1);
	
	fprintf(stderr, "ptr1 = %p\nptr2 = %p\n", ptr1, ptr2);
	
	fprintf(stderr, "dump_mem(ptr1)\n");
	dump_mem(ptr1, 0x200);
	fprintf(stderr, "dump_mem(ptr2)\n");
	dump_mem(ptr2, 0x200);
	
	for (uint8_t *ptr = ptr1; ptr != (uint8_t *)ptr1 + 0x200; ++ptr) {
		*ptr = ~(*ptr);
	}
	
	if (memcmp(ptr1, ptr2, 0x200) != 0) {
		errx(1, "%s: memcmp says ptr1[x] != ptr2[x]", __func__);
	}
	
	fprintf(stderr, "dump_mem(ptr1)\n");
	dump_mem(ptr1, 0x200);
	fprintf(stderr, "dump_mem(ptr2)\n");
	dump_mem(ptr2, 0x200);
	
	dev_unmap_sect(ptr1, 0, 1);
	
	fprintf(stderr, "dump_mem(ptr2)\n");
	dump_mem(ptr2, 0x200);
	fprintf(stderr, "dump_mem(ptr1)\n");
	dump_mem(ptr1, 0x200);
	
	dev_close();
}
