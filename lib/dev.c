#include "dev.h"
#include <fcntl.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <unistd.h>
#include "debug.h"


struct jgfs2_dev dev;
static struct jgfs2_dev dev_init = {
	.read_only = true,
	
	.page_size = 0,
	
	.path = NULL,
	.fd   = -1,
	
	.size_byte = 0,
	.size_sect = 0,
	
	.map_cnt = 0,
};


void *jgfs2_dev_map_sect(uint32_t sect_num, uint32_t sect_cnt) {
	if (sect_num + sect_cnt >= dev.size_sect) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") >= %" PRIu32,
			__func__, sect_num, sect_num + sect_cnt, dev.size_sect);
	}
	
	uint64_t byte_off = SECT_TO_BYTE(sect_num);
	uint64_t byte_len = SECT_TO_BYTE(sect_cnt);
	
	/* if not page-aligned, make it so */
	uint32_t adjust = (byte_off % dev.page_size);
	if (adjust != 0) {
		byte_len += adjust;
		byte_off /= dev.page_size;
	}
	
	int prot = PROT_READ | (dev.read_only ? 0 : PROT_WRITE);
	void *addr = mmap(NULL, byte_len, prot, MAP_SHARED, dev.fd, byte_off);
	
	if (addr == MAP_FAILED) {
		err(1, "%s: mmap failed: sect [%" PRIu32 ", %" PRIu32 ")",
			__func__, sect_num, sect_num + sect_cnt);
	}
	
	++dev.map_cnt;
	
	return (addr + adjust);
}

void jgfs2_dev_unmap_sect(void *addr, uint32_t sect_num, uint32_t sect_cnt) {
	if (sect_num + sect_cnt >= dev.size_sect) {
		errx(1, "%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") >= %" PRIu32,
			__func__, sect_num, sect_num + sect_cnt, dev.size_sect);
	}
	
	uint64_t byte_off = SECT_TO_BYTE(sect_num);
	uint64_t byte_len = SECT_TO_BYTE(sect_cnt);
	
	/* if not page-aligned, make it so */
	uint32_t adjust = (byte_off % dev.page_size);
	if (adjust != 0) {
		byte_len += adjust;
		byte_off /= dev.page_size;
		
		addr -= adjust;
	}
	
	if (munmap(addr, byte_len) < 0) {
		err(1, "%s: munmap failed: addr %p, sect [%" PRIu32 ", %" PRIu32 ")",
			__func__, addr, sect_num, sect_num + sect_cnt);
	}
	
	--dev.map_cnt;
}

void jgfs2_dev_fsync(void) {
	if (fsync(dev.fd) < 0) {
		warn("fsync failed");
	}
}

void jgfs2_dev_msync(void *addr, size_t length) {
	if (msync(addr, length, MS_SYNC) < 0) {
		warn("msync failed");
	}
}

void jgfs2_dev_open(const char *dev_path, bool read_only) {
	dev = dev_init;
	
	if ((dev.page_size = sysconf(_SC_PAGESIZE)) < 0) {
		err(1, "could not get system page size");
	}
	
	dev.read_only = read_only;
	
	dev.path = strdup(dev_path);
	int flags = (dev.read_only ? O_RDONLY : O_RDWR);
	if ((dev.fd = open(dev.path, flags)) < 0) {
		err(1, "failed to open '%s'", dev.path);
	}
	
	dev.size_byte = lseek(dev.fd, 0, SEEK_END);
	lseek(dev.fd, 0, SEEK_SET);
	
	dev.size_sect = dev.size_byte / JGFS2_SECT_SIZE;
}

void jgfs2_dev_close(void) {
	if (dev.fd != -1) {
		jgfs2_dev_fsync();
		
		if (dev.map_cnt != 0) {
			warnx("%" PRIu32 " device regions are still mapped", dev.map_cnt);
		}
		
		if (close(dev.fd) < 0) {
			warn("close failed");
		}
		
		dev.fd = -1;
	}
	
	dev.path = NULL;
}
