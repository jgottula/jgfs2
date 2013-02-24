#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include "../../lib/blk.h"
#include "../../lib/dev.h"
#include "../../lib/fs.h"


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

static void test_bmap(void) {
	warnx(__func__);
	
	jgfs2_init("/dev/loop0p1", &(struct jgfs2_mount_options){false});
	
	memset(fs.free_bmap, 0, fs.free_bmap_size_byte);
	fprintf(stderr, "zero:\n");
	dump(fs.free_bmap, fs.free_bmap_size_byte);
	
	jgfs2_blk_bmap_set(true, 7, 64);
	fprintf(stderr, "7 thru 70 inclusive:\n");
	dump(fs.free_bmap, fs.free_bmap_size_byte);
	
	for (uint32_t i = 0; i < fs.size_blk; ++i) {
		bool isfree = jgfs2_blk_bmap_isfree(i, 1);
		
		assert(i >= 7 || isfree);
		assert((i < 7 || i > 70) || !isfree);
		assert(i <= 70 || isfree);
	}
	
	jgfs2_done();
}

int main(int argc, char **argv) {
	warnx("performing unit tests");
	
	//test_mmap();
	test_bmap();
}
