/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../../lib/jgfs2.h"
#include "../../lib/debug.h"
#include "../../lib/dev.h"


void test_mmap(void) {
	fprintf(stderr, "%s\n", __func__);
	
	dev_open("/dev/loop0p1", false, true);
	
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
		fprintf(stderr, "%s: memcmp says ptr1[x] != ptr2[x]\n", __func__);
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
