/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "dev.h"
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>
#include "debug.h"


struct dev dev;
static struct dev dev_null = {
	.read_only = true,
	.debug_map = false,
	
	.page_size = 0,
	
	.path = NULL,
	.fd   = -1,
	
	.size_byte = 0,
	.size_sect = 0,
	
	.map_cnt = 0,
};


void *dev_map(uint32_t sect_num, uint32_t sect_cnt, bool writable) {
	if (sect_num + sect_cnt > dev.size_sect) {
		errx("%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") > %" PRIu32,
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
	
	int prot = PROT_READ;
	if (writable) {
		if (!dev.read_only) {
			prot |= PROT_WRITE;
		} else {
			errx("%s: wanted write on ro dev: sect [%" PRIu32 ", %" PRIu32 ")",
				__func__, sect_num, sect_num + sect_cnt);
		}
	}
	
	void *addr = mmap(NULL, byte_len, prot, MAP_SHARED, dev.fd, byte_off);
	
	if (addr == MAP_FAILED) {
		err("%s: mmap failed: sect [%" PRIu32 ", %" PRIu32 ")",
			__func__, sect_num, sect_num + sect_cnt);
	}
	
	++dev.map_cnt;
	
	if (dev.debug_map) {
		debug_map_push(addr, sect_num, sect_cnt);
	}
	
	return (addr + adjust);
}

void dev_unmap(void *addr, uint32_t sect_num, uint32_t sect_cnt) {
	if (sect_num + sect_cnt > dev.size_sect) {
		errx("%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") > %" PRIu32,
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
		err("%s: munmap failed: addr %p, sect [%" PRIu32 ", %" PRIu32 ")",
			__func__, addr, sect_num, sect_num + sect_cnt);
	}
	
	--dev.map_cnt;
	
	if (dev.debug_map) {
		debug_map_pop(addr, sect_num, sect_cnt);
	}
}

void dev_msync(void *addr, uint32_t sect_num, uint32_t sect_cnt, bool async) {
	if (sect_num + sect_cnt > dev.size_sect) {
		errx("%s: bounds violation: [%" PRIu32 ", %" PRIu32 ") > %" PRIu32,
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
	
	int flags = (async ? MS_ASYNC : MS_SYNC);
	if (msync(addr, byte_len, flags)) {
		err("%s: msync failed: sect [%" PRIu32 ", %" PRIu32 ")",
			__func__, sect_num, sect_num + sect_cnt);
	}
}

void dev_fsync(void) {
	if (fsync(dev.fd) < 0) {
		warn("fsync failed");
	}
}

void dev_open(const char *dev_path, bool read_only, bool debug_map) {
	dev = dev_null;
	
	if ((dev.page_size = sysconf(_SC_PAGESIZE)) < 0) {
		err("could not get system page size");
	}
	
	dev.read_only = read_only;
	dev.debug_map = debug_map;
	
	dev.path = strdup(dev_path);
	int flags = (dev.read_only ? O_RDONLY : O_RDWR);
	if ((dev.fd = open(dev.path, flags)) < 0) {
		err("failed to open '%s'", dev.path);
	}
	
	if (flock(dev.fd, LOCK_NB | LOCK_EX) < 0) {
		err("could not lock '%s'", dev.path);
	}
	
	dev.size_byte = lseek(dev.fd, 0, SEEK_END);
	lseek(dev.fd, 0, SEEK_SET);
	
	dev.size_sect = dev.size_byte / JGFS2_SECT_SIZE;
}

void dev_close(void) {
	if (dev.fd != -1) {
		dev_fsync();
		
		if (dev.map_cnt != 0) {
			warnx("%" PRIu32 " device regions are still mapped", dev.map_cnt);
			
			if (dev.debug_map) {
				debug_map_dump();
			}
		}
		
		if (flock(dev.fd, LOCK_NB | LOCK_UN) < 0) {
			warn("could not unlock '%s'", dev.path);
		}
		if (close(dev.fd) < 0) {
			warn("failed to close '%s'", dev.path);
		}
		
		dev.fd = -1;
	}
	
	dev.path = NULL;
}
