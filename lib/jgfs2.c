#include "jgfs2.h"
#include <bsd/string.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <uuid/uuid.h>


static struct {
	const char *path;
	int         fd;
	void       *mem;
	
	uint64_t mmap_size;
	uint64_t size;
	uint64_t sect;
} dev = {
	.path = NULL,
	.fd   = -1,
	.mem  = MAP_FAILED,
	
	.mmap_size = 0,
	.size      = 0,
	.sect      = 0,
};

static struct {
	bool init;
	
	struct jgfs2_mount_options mount_opt;
	
	uint64_t size;
	
	uint32_t block_size;
	uint32_t block_count;
	
	uint32_t first_data_block;
	
	struct jgfs2_sect       *vbr;
	struct jgfs2_superblock *sblk;
	struct jgfs2_sect       *boot;
} fs = {
	.init = false,
	
	.size = 0,
	
	.block_size  = 0,
	.block_count = 0,
	
	.first_data_block = 0,
	
	.vbr  = NULL,
	.sblk = NULL,
	.boot = NULL,
};


static void err(int eval, const char *fmt, ...) {
	fputs("jgfs2: ", stderr);
	
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	
	fprintf(stderr, ": %s\n", strerror(errno));
	
	exit(eval);
}

static void errx(int eval, const char *fmt, ...) {
	fputs("jgfs2: ", stderr);
	
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	
	fputc('\n', stderr);
	
	exit(eval);
}

static void warn(const char *fmt, ...) {
	fputs("jgfs2: ", stderr);
	
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	
	fprintf(stderr, ": %s\n", strerror(errno));
}

static void warnx(const char *fmt, ...) {
	fputs("jgfs2: ", stderr);
	
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	
	fputc('\n', stderr);
}

static void *jgfs2_sect_ptr(uint64_t sect_num) {
	if (sect_num >= dev.sect) {
		errx(1, "%s: bounds violation (%" PRIu64 " >= %" PRIu64 ")",
			__func__, sect_num, dev.sect);
	}
	
	return (void *)((struct jgfs2_sect *)dev.mem + sect_num);
}

static void *jgfs2_block_ptr(uint32_t block_num) {
	if (block_num >= fs.block_count) {
		errx(1, "%s: bounds violation (%" PRIu32 " >= %" PRIu32 ")",
			__func__, block_num, fs.block_count);
	}
	
	return jgfs2_sect_ptr(block_num * fs.sblk->s_block_size);
}

static void jgfs2_msync(void) {
	if (msync(dev.mem, dev.mmap_size, MS_SYNC) < 0) {
		warn("msync failed");
	}
}

static void jgfs2_fsync(void) {
	if (fsync(dev.fd) < 0) {
		warn("fsync failed");
	}
}

static void jgfs2_clean_up(void) {
	if (dev.mem != MAP_FAILED) {
		jgfs2_msync();
		
		if (munmap(dev.mem, dev.mmap_size) < 0) {
			warn("munmap failed");
		}
		dev.mem = MAP_FAILED;
	}
	
	if (dev.fd != -1) {
		jgfs2_fsync();
		
		if (close(dev.fd) < 0) {
			warn("close failed");
		}
		dev.fd = -1;
	}
	
	fs.init = false;
}

static void jgfs2_init_real(const struct jgfs2_superblock *new_sblk) {
	warnx("version 0x%04x", JGFS2_VER_TOTAL);
	
	atexit(jgfs2_clean_up);
	
	int flags = (fs.mount_opt.read_only ? O_RDONLY : O_RDWR);
	if ((dev.fd = open(dev.path, flags)) < 0) {
		err(1, "failed to open '%s'", dev.path);
	}
	
	dev.size = lseek(dev.fd, 0, SEEK_END);
	lseek(dev.fd, 0, SEEK_SET);
	
	dev.sect = dev.size / JGFS2_SECT_SIZE;
	
	warnx("TODO: device size checks");
	
	fs.size = (JGFS2_SBLK_SECT + 1) * JGFS2_SECT_SIZE;
	
	/* mmap just the superblock at first */
	int prot = PROT_READ | (fs.mount_opt.read_only ? 0 : PROT_WRITE);
	if ((dev.mem = mmap(NULL, fs.size, prot, MAP_SHARED,
		dev.fd, 0)) == MAP_FAILED) {
		err(1, "mmap failed");
	}
	dev.mmap_size = fs.size;
	
	fs.vbr  = jgfs2_sect_ptr(JGFS2_VBR_SECT);
	fs.sblk = jgfs2_sect_ptr(JGFS2_SBLK_SECT);
	
	if (new_sblk != NULL) {
		warnx("TODO: write all superblocks");
		
		memcpy(fs.sblk, new_sblk, sizeof(*new_sblk));
	}
	
	if (memcmp(fs.sblk->s_magic, JGFS2_MAGIC,
		sizeof(fs.sblk->s_magic)) != 0) {
		warnx("TODO: check backup superblocks");
		/* when checking backup superblocks, be aware that the mmap doesn't yet
		 * extend that far into the device */
		
		errx(1, "not jgfs2 or invalid superblock");
	}
	
	if (fs.sblk->s_ver_major != JGFS2_VER_MAJOR ||
		fs.sblk->s_ver_minor != JGFS2_VER_MINOR) {
		errx(1, "incompatible version (0x%04x)",
			JGFS2_VER_EXPAND(fs.sblk->s_ver_major, fs.sblk->s_ver_minor));
	}
	
	if (fs.sblk->s_sect_count > dev.sect) {
		errx(1, "filesystem exceeds device bounds (%"PRIu64 " > %" PRIu64 ")",
			fs.sblk->s_sect_count, dev.sect);
	}
	
	fs.size = fs.sblk->s_sect_count * JGFS2_SECT_SIZE;
	
	/* set the mmap size to the actual size of the filesystem */
	if (mremap(dev.mem, dev.mmap_size, fs.size, 0) == MAP_FAILED) {
		err(1, "mremap failed");
	}
	dev.mmap_size = fs.size;
	
	fs.boot = jgfs2_sect_ptr(JGFS2_BOOT_SECT);
	
	fs.block_size  = fs.sblk->s_block_size * JGFS2_SECT_SIZE;
	fs.block_count = dev.size / fs.block_size;
	
	fs.first_data_block = CEIL(JGFS2_BOOT_SECT + fs.sblk->s_boot_size,
		fs.block_size);
	
	if (fs.sblk->s_mtime > time(NULL)) {
		warnx("last mount time is in the future: %s",
			ctime((const time_t *)&fs.sblk->s_mtime));
	}
	
	if (!fs.mount_opt.read_only) {
		fs.sblk->s_mtime = time(NULL);
	}
	
	fs.init = true;
}

void jgfs2_init(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt) {
	dev.path = strdup(dev_path);
	memcpy(&fs.mount_opt, mount_opt, sizeof(*mount_opt));
	
	jgfs2_init_real(NULL);
}

void jgfs2_new(const char *dev_path,
	const struct jgfs2_mount_options *mount_opt,
	const struct jgfs2_mkfs_param *param) {
	struct jgfs2_mkfs_param param_rw;
	memcpy(&param_rw, param, sizeof(param_rw));
	
	warnx("making new filesystem with label '%s' and uuid '%s'",
		param_rw.label);
	
	if (param_rw.sect_count == 0) {
		warnx("using entire device");
		
		int temp_fd;
		if ((temp_fd = open(dev.path, O_RDONLY)) < 0) {
			err(1, "failed to open '%s'", dev.path);
		}
		
		param_rw.sect_count = lseek(temp_fd, 0, SEEK_END) / JGFS2_SECT_SIZE;
		
		if (close(temp_fd) < 0) {
			err(1, "failed to close '%s'", dev.path);
		}
	}
	
	if (param_rw.block_size == 0) {
		/* advanced block size choosing algorithm */
		param_rw.block_size = 2;
		
		warnx("using best block size: %" PRIu32 " byte blocks");
	}
	
	warnx("TODO: device size checks");
	/* note that not all size variables have been initialized at this point */
	
	struct jgfs2_superblock new_sblk;
	memset(&new_sblk, 0, sizeof(new_sblk));
	
	memcpy(new_sblk.s_magic, JGFS2_MAGIC, sizeof(new_sblk.s_magic));
	
	new_sblk.s_ver_major = JGFS2_VER_MAJOR;
	new_sblk.s_ver_minor = JGFS2_VER_MINOR;
	
	new_sblk.s_sect_count = param_rw.sect_count;
	new_sblk.s_boot_size  = param_rw.boot_size;
	
	new_sblk.s_block_size = param_rw.block_size;
	
	new_sblk.s_ctime = time(NULL);
	new_sblk.s_mtime = 0;
	
	memcpy(new_sblk.s_uuid, param_rw.uuid, sizeof(new_sblk.s_uuid));
	
	strlcpy(new_sblk.s_label, param_rw.label, sizeof(new_sblk.s_label));
	
	jgfs2_init_real(&new_sblk);
	
	if (param_rw.zap_vbr) {
		warnx("zapping the volume boot record");
		
		struct jgfs2_sect *vbr = jgfs2_sect_ptr(JGFS2_VBR_SECT);
		memset(vbr, 0, JGFS2_SECT_SIZE);
	}
	
	if (param_rw.zap_boot) {
		warnx("zapping the boot area");
		
		struct jgfs2_sect *boot = jgfs2_sect_ptr(JGFS2_BOOT_SECT);
		memset(boot, 0, fs.sblk->s_boot_size * JGFS2_SECT_SIZE);
	}
	
	/* always zap the slack space between the end of the boot area and the first
	 * usable block */
	struct jgfs2_sect *slack =
		jgfs2_sect_ptr(JGFS2_BOOT_SECT + fs.sblk->s_boot_size);
	memset(slack, 0, (fs.first_data_block * fs.sblk->s_block_size) -
		(JGFS2_BOOT_SECT + fs.sblk->s_boot_size));
	
	if (param_rw.zap_data) {
		warnx("zapping all data blocks");
		
		void *data = jgfs2_block_ptr(fs.first_data_block);
		memset(data, 0, fs.block_count - fs.first_data_block);
	}
	
	warnx("TODO: initialize fs structures");
	warnx("TODO: set free space bitmap");
}
