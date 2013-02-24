#include <err.h>
#include <stdio.h>
#include <string.h>
#include "../../lib/dev.h"


static void dump(void *ptr, size_t len) {
	uint8_t *data = ptr;
	uint32_t i = 0;
	while (len-- != 0) {
		fprintf(stderr, " %02x", *(data++));
		
		if (++i == 16) {
			fputc('\n', stderr);
			
			i = 0;
		}
	}
}

static void test_mmap(void) {
	warnx(__func__);
	
	jgfs2_dev_open("/dev/loop0p1", false);
	
	void *ptr1 = jgfs2_dev_map_sect(0, 1);
	void *ptr2 = jgfs2_dev_map_sect(0, 1);
	
	fprintf(stderr, "ptr1 = %p\nptr2 = %p\n", ptr1, ptr2);
	
	fprintf(stderr, "dump(ptr1)\n");
	dump(ptr1, 0x200);
	fprintf(stderr, "dump(ptr2)\n");
	dump(ptr2, 0x200);
	
	for (uint8_t *ptr = ptr1; ptr != (uint8_t *)ptr1 + 0x200; ++ptr) {
		*ptr = ~(*ptr);
	}
	
	if (memcmp(ptr1, ptr2, 0x200) != 0) {
		errx(1, "%s: memcmp says ptr1[x] != ptr2[x]", __func__);
	}
	
	fprintf(stderr, "dump(ptr1)\n");
	dump(ptr1, 0x200);
	fprintf(stderr, "dump(ptr2)\n");
	dump(ptr2, 0x200);
	
	jgfs2_dev_unmap_sect(ptr1, 0, 1);
	
	fprintf(stderr, "dump(ptr2)\n");
	dump(ptr2, 0x200);
	fprintf(stderr, "dump(ptr1)\n");
	dump(ptr1, 0x200);
	
	jgfs2_dev_close();
}

int main(int argc, char **argv) {
	warnx("performing unit tests");
	
	test_mmap();
}
