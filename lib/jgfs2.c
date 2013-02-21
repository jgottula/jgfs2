#include "jgfs2.h"
#include <bsd/string.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>


static int      dev_fd   = -1;
static void    *dev_mem  = NULL;
static uint64_t dev_size = 0;
static uint64_t dev_sect = 0;

struct jgfs jgfs = {
	.hdr  = NULL,
	.boot = NULL,
	.fat  = NULL,
};

static uint16_t fs_clusters = 0;


static void jgfs_msync(void) {
	if (msync(dev_mem, dev_size, MS_SYNC) == -1) {
		warn("msync failed");
	}
}

static void jgfs_fsync(void) {
	if (fsync(dev_fd) == -1) {
		warn("fsync failed");
	}
}

static void jgfs_clean_up(void) {
	if (dev_mem != NULL) {
		jgfs_msync();
		
		if (munmap(dev_mem, dev_size) == -1) {
			warn("munmap failed");
		}
		dev_mem = NULL;
	}
	
	if (dev_fd != -1) {
		jgfs_fsync();
		
		if (close(dev_fd) == -1) {
			warn("close failed");
		}
		dev_fd = -1;
	}
}

static void jgfs_init_real(const char *dev_path,
	const struct jgfs_hdr *new_hdr) {
	warnx("using jgfs version 0x%02x%02x", JGFS_VER_MAJOR, JGFS_VER_MINOR);
	
	atexit(jgfs_clean_up);
	
	if ((dev_fd = open(dev_path, O_RDWR)) == -1) {
		err(1, "failed to open '%s'", dev_path);
	}
	
	dev_size = lseek(dev_fd, 0, SEEK_END);
	lseek(dev_fd, 0, SEEK_SET);
	
	dev_sect = dev_size / SECT_SIZE;
	
	if (dev_sect < 2) {
		errx(1, "device has less than two sectors");
	} else if (dev_size % 512 != 0) {
		warnx("device has non-integer number of sectors");
	}
	
	if ((dev_mem = mmap(NULL, dev_size, PROT_READ | PROT_WRITE, MAP_SHARED,
		dev_fd, 0)) == MAP_FAILED) {
		err(1, "mmap failed");
	}
	
	jgfs.hdr = jgfs_get_sect(JGFS_HDR_SECT);
	
	if (new_hdr != NULL) {
		memset(jgfs.hdr, 0, SECT_SIZE);
		memcpy(jgfs.hdr, new_hdr, sizeof(*jgfs.hdr));
	}
	
	if (memcmp(jgfs.hdr->magic, JGFS_MAGIC, sizeof(jgfs.hdr->magic)) != 0) {
		errx(1, "jgfs header not found");
	}
	
	if (jgfs.hdr->ver_major != JGFS_VER_MAJOR ||
		jgfs.hdr->ver_minor != JGFS_VER_MINOR) {
		errx(1, "incompatible filesystem (%#06" PRIx16 ")",
			JGFS_VER_EXPAND(jgfs.hdr->ver_major, jgfs.hdr->ver_minor));
	}
	
	if (dev_sect < jgfs.hdr->s_total) {
		errx(1, "filesystem exceeds device bounds (%" PRIu32 " > %" PRIu64 ")",
			jgfs.hdr->s_total, dev_sect);
	}
	
	jgfs.boot = jgfs_get_sect(JGFS_BOOT_SECT);
	jgfs.fat  = jgfs_get_sect(JGFS_BOOT_SECT + jgfs.hdr->s_boot);
	
	fs_clusters = (jgfs.hdr->s_total -
		(2 + jgfs.hdr->s_boot + jgfs.hdr->s_fat)) / jgfs.hdr->s_per_c;
	
	if (jgfs.hdr->s_fat < CEIL(fs_clusters, JGFS_FENT_PER_S)) {
		errx(1, "fat is too small");
	}
	
	if (jgfs.hdr->mtime > time(NULL)) {
		warnx("last mount time is in the future");
	}
	
	jgfs.hdr->mtime = time(NULL);
}

void jgfs_init(const char *dev_path) {
	jgfs_init_real(dev_path, NULL);
}

void jgfs_new(const char *dev_path, struct jgfs_mkfs_param *param) {
	warnx("making new jgfs with label '%s'", param->label);
	
	if (param->s_total == 0) {
		warnx("using entire device");
		
		int temp_fd;
		if ((temp_fd = open(dev_path, O_RDONLY)) == -1) {
			err(1, "failed to open '%s'", dev_path);
		}
		
		param->s_total = lseek(temp_fd, 0, SEEK_END) / SECT_SIZE;
		close(temp_fd);
	}
	
	if (param->s_per_c == 0) {
		param->s_per_c = 1;
		while (param->s_total / param->s_per_c > FAT_LAST + 1) {
			param->s_per_c *= 2;
		}
		
		warnx("using best cluster size: %u byte clusters",
			param->s_per_c * SECT_SIZE);
	}
	
	uint16_t s_vbr_hdr_boot = 2 + param->s_boot;
	
	if (param->s_total < 2) {
		errx(1, "filesystem must have at least 2 sectors");
	} else if (param->s_total < s_vbr_hdr_boot) {
		errx(1, "filesystem is too small for the boot area requested");
	} else if (param->s_total == s_vbr_hdr_boot) {
		errx(1, "filesystem has no room for a fat");
	}
	
	struct jgfs_hdr new_hdr;
	
	memset(&new_hdr, 0, sizeof(new_hdr));
	
	memcpy(new_hdr.magic, JGFS_MAGIC, sizeof(new_hdr.magic));
	new_hdr.ver_major = JGFS_VER_MAJOR;
	new_hdr.ver_minor = JGFS_VER_MINOR;
	
	new_hdr.s_total = param->s_total;
	new_hdr.s_boot  = param->s_boot;
	
	new_hdr.s_per_c = param->s_per_c;
	
	/* iteratively calculate optimal fat size, taking into account the size of
	 * the fat itself when determining the number of available clusters */
	/* TODO: fix infinite loop when too small of a cluster size is manually
	 * specified */
	uint16_t s_fat = 1, last = 0xffff, last_last;
	do {
		last_last = last;
		last = s_fat;
		
		s_fat = CEIL(new_hdr.s_total - (2 + new_hdr.s_boot + new_hdr.s_fat),
			JGFS_FENT_PER_S * new_hdr.s_per_c);
		
		/* if we are vacillating infinitely between two possible fat sizes, just
		 * pick the larger size and be done with it */
		if (s_fat != last && s_fat == last_last) {
			if (s_fat < last) {
				s_fat = last;
			}
			break;
		}
	} while (s_fat != last);
	
	new_hdr.s_fat = s_fat;
	
	new_hdr.ctime = time(NULL);
	new_hdr.mtime = 0;
	
	strlcpy(new_hdr.label, param->label, JGFS_LABEL_LIMIT + 1);
	
	new_hdr.root_dir_ent.type  = TYPE_DIR;
	new_hdr.root_dir_ent.mtime = time(NULL);
	new_hdr.root_dir_ent.size  = SECT_SIZE * param->s_per_c;
	new_hdr.root_dir_ent.begin = FAT_ROOT;
	
	jgfs_init_real(dev_path, &new_hdr);
	
	/* initialize the fat */
	for (uint16_t i = 0; i < JGFS_FENT_PER_S * jgfs.hdr->s_fat; ++i) {
		fat_ent_t *entry =
			&jgfs.fat[i / JGFS_FENT_PER_S].entries[i % JGFS_FENT_PER_S];
		
		if (i < fs_clusters) {
			*entry = FAT_FREE;
		} else {
			*entry = FAT_OOB;
		}
	}
	
	if (param->zap) {
		warnx("zapping the vbr and boot area");
		
		struct sect *sect = jgfs_get_sect(JGFS_VBR_SECT);
		memset(sect, 0, SECT_SIZE);
		
		for (uint16_t i = 0; i < jgfs.hdr->s_boot; ++i) {
			sect = jgfs_get_sect(JGFS_BOOT_SECT + i);
			memset(sect, 0, SECT_SIZE);
		}
	}
	
	if (param->zero_data) {
		warnx("zeroing out data clusters");
		
		for (uint16_t i = 0; i < fs_clusters; ++i) {
			void *clust = jgfs_get_clust(i);
			
			memset(clust, 0, jgfs_clust_size());
		}
	}
	
	if (jgfs_fs_clusters() == 0) {
		errx(1, "filesystem has no room for a root directory");
	}
	
	/* initialize the root directory cluster */
	struct jgfs_dir_clust *root_dir_clust = jgfs_get_clust(FAT_ROOT);
	jgfs_dir_init(root_dir_clust);
	
	jgfs_fat_write(FAT_ROOT, FAT_EOF);
}

void jgfs_done(void) {
	jgfs_clean_up();
}

void jgfs_sync(void) {
	jgfs_msync();
	jgfs_fsync();
}

uint32_t jgfs_clust_size(void) {
	return (SECT_SIZE * jgfs.hdr->s_per_c);
}

uint16_t jgfs_fs_clusters(void) {
	return fs_clusters;
}

void *jgfs_get_sect(uint32_t sect_num) {
	if (sect_num >= dev_sect) {
		errx(1, "jgfs_get_sect: tried to access past end of device "
			"(sect %" PRIu32 ")", sect_num);
	}
	
	return (void *)((struct sect *)dev_mem + sect_num);
}

void *jgfs_get_clust(fat_ent_t clust_num) {
	if (clust_num > FAT_LAST) {
		errx(1, "jgfs_get_clust: tried to access past FAT_LAST "
			"(clust %#06" PRIx16 ")", clust_num);
	} else if (clust_num >= fs_clusters) {
		errx(1, "jgfs_get_clust: tried to access nonexistent cluster "
			"(clust %#06" PRIx16 ")", clust_num);
	}
	
	return jgfs_get_sect(2 + jgfs.hdr->s_boot + jgfs.hdr->s_fat +
		(clust_num * jgfs.hdr->s_per_c));
}

fat_ent_t jgfs_fat_read(fat_ent_t addr) {
	uint16_t fat_sect = addr / JGFS_FENT_PER_S;
	uint16_t fat_idx  = addr % JGFS_FENT_PER_S;
	
	if (fat_sect >= jgfs.hdr->s_fat) {
		errx(1, "jgfs_fat_read: tried to access past s_fat "
			"(fat %#06" PRIx16 ")", addr);
	} else if (addr >= fs_clusters) {
		errx(1, "jgfs_fat_read: tried to access non-real entry "
			"(fat %#06" PRIx16 ")", addr);
	}
	
	return jgfs.fat[fat_sect].entries[fat_idx];
}

void jgfs_fat_write(fat_ent_t addr, fat_ent_t val) {
	uint16_t fat_sect = addr / JGFS_FENT_PER_S;
	uint16_t fat_idx  = addr % JGFS_FENT_PER_S;
	
	if (fat_sect >= jgfs.hdr->s_fat) {
		errx(1, "jgfs_fat_write: tried to access past s_fat "
			"(fat %#06" PRIx16 ")", addr);
	} else if (addr >= fs_clusters) {
		errx(1, "jgfs_fat_write: tried to access non-real entry "
			"(fat %#06" PRIx16 ")", addr);
	}
	
	jgfs.fat[fat_sect].entries[fat_idx] = val;
}

bool jgfs_fat_find(fat_ent_t target, fat_ent_t *first) {
	for (uint16_t i = 0; i < jgfs.hdr->s_fat; ++i) {
		for (uint16_t j = 0; j < JGFS_FENT_PER_S; ++j) {
			if (jgfs.fat[i].entries[j] == target) {
				*first = (i * JGFS_FENT_PER_S) + j;
				return true;
			}
		}
	}
	
	return false;
}

uint16_t jgfs_fat_count(fat_ent_t target) {
	uint16_t count = 0;
	
	for (uint16_t i = 0; i < jgfs.hdr->s_fat; ++i) {
		for (uint16_t j = 0; j < JGFS_FENT_PER_S; ++j) {
			if (jgfs.fat[i].entries[j] == target) {
				++count;
			}
		}
	}
	
	return count;
}

void jgfs_fat_dump(void) {
	for (uint16_t i = 0; i < jgfs.hdr->s_fat; ++i) {
		for (uint16_t j = 0; j < JGFS_FENT_PER_S; ++j) {
			if (j % 8 == 0) {
				fprintf(stderr, "%04lx:", j + (i * JGFS_FENT_PER_S));
			}
			
			fprintf(stderr, " %04" PRIx16, jgfs.fat[i].entries[j]);
			
			if (j % 8 == 7) {
				fputc('\n', stderr);
			}
		}
	}
}

int jgfs_lookup(const char *path, struct jgfs_dir_clust **parent,
	struct jgfs_dir_ent **child) {
	bool find_child = (child != NULL);
	
	struct jgfs_dir_clust *dir_clust;
	struct jgfs_dir_ent   *dir_ent;
	
	dir_clust = jgfs_get_clust(FAT_ROOT);
	dir_ent   = &jgfs.hdr->root_dir_ent;
	
	char *strtok_save = NULL;
	char *path_dup = strdup(path);
	char *path_part = strtok_r(path_dup, "/", &strtok_save);
	char *path_next;
	
	/* bail out before the last path component, but only if we weren't asked to
	 * find the child (we always at least find the parent) */
	while (path_part != NULL &&
		(path_next = strtok_r(NULL, "/", &strtok_save),
		find_child || path_next != NULL)) {
		
		if (jgfs_lookup_child(path_part, dir_clust, &dir_ent) != 0) {
			free(path_dup);
			return -ENOENT;
		}
		
		/* if we're still getting to the child, make sure we don't try to
		 * recurse into a non-directory */
		if (path_next != NULL) {
			if (dir_ent->type != TYPE_DIR) {
				return -ENOTDIR;
			}
			
			dir_clust = jgfs_get_clust(dir_ent->begin);
		}
		
		path_part = path_next;
	}
	
	/* only assign to output pointers on success */
	*parent = dir_clust;
	if (find_child) {
		*child = dir_ent;
	}
	
	free(path_dup);
	return 0;
}

int jgfs_lookup_child(const char *name, struct jgfs_dir_clust *parent,
	struct jgfs_dir_ent **child) {
	
	for (struct jgfs_dir_ent *this_ent = parent->entries;
		this_ent < parent->entries + JGFS_DENT_PER_C; ++this_ent) {
		if (strncmp(this_ent->name, name, JGFS_NAME_LIMIT + 1) == 0) {
			*child = this_ent;
			return 0;
		}
	}
	
	return -ENOENT;
}

void jgfs_dir_init(struct jgfs_dir_clust *dir_clust) {
	memset(dir_clust, 0, jgfs_clust_size());
}

uint32_t jgfs_dir_count(struct jgfs_dir_clust *dir_clust) {
	uint32_t count = 0;
	for (struct jgfs_dir_ent *this_ent = dir_clust->entries;
		this_ent < dir_clust->entries + JGFS_DENT_PER_C; ++this_ent) {
		if (this_ent->name[0] != '\0') {
			++count;
		}
	}
	
	return count;
}

int jgfs_dir_foreach(jgfs_dir_func_t func, struct jgfs_dir_clust *dir_clust,
	void *user_ptr) {
	for (struct jgfs_dir_ent *this_ent = dir_clust->entries;
		this_ent < dir_clust->entries + JGFS_DENT_PER_C; ++this_ent) {
		if (this_ent->name[0] != '\0') {
			int rtn;
			if ((rtn = func(this_ent, user_ptr)) != 0) {
				return rtn;
			}
		}
	}
	
	return 0;
}

int jgfs_create_ent(struct jgfs_dir_clust *parent,
	const struct jgfs_dir_ent *new_ent, struct jgfs_dir_ent **created_ent) {
	if (strlen(new_ent->name) == 0) {
		errx(1, "jgfs_create_ent: new_ent->name is empty");
	}
	
	struct jgfs_dir_ent *extant_ent;
	if (jgfs_lookup_child(new_ent->name, parent, &extant_ent) == 0) {
		return -EEXIST;
	}
	
	struct jgfs_dir_ent *avail_ent;
	for (struct jgfs_dir_ent *this_ent = parent->entries;
		this_ent < parent->entries + JGFS_DENT_PER_C; ++this_ent) {
		if (this_ent->name[0] == '\0') {
			avail_ent = this_ent;
			goto found;
		}
	}
	
	return -ENOSPC;
	
found:
	memcpy(avail_ent, new_ent, sizeof(*avail_ent));
	
	if (created_ent != NULL) {
		*created_ent = avail_ent;
	}
	
	return 0;
}

int jgfs_create_file(struct jgfs_dir_clust *parent, const char *name) {
	if (strlen(name) > JGFS_NAME_LIMIT) {
		return -ENAMETOOLONG;
	}
	
	struct jgfs_dir_ent new_ent;
	memset(&new_ent, 0, sizeof(new_ent));
	strlcpy(new_ent.name, name, JGFS_NAME_LIMIT + 1);
	new_ent.type  = TYPE_FILE;
	new_ent.attr  = ATTR_NONE;
	new_ent.mtime = time(NULL);
	new_ent.size  = 0;
	new_ent.begin = FAT_NALLOC;
	
	return jgfs_create_ent(parent, &new_ent, NULL);
}

int jgfs_create_dir(struct jgfs_dir_clust *parent, const char *name) {
	if (strlen(name) > JGFS_NAME_LIMIT) {
		return -ENAMETOOLONG;
	}
	
	fat_ent_t dest_addr;
	/* make sure a free cluster exists before we bother adding a dir ent to the
	 * parent directory */
	if (!jgfs_fat_find(FAT_FREE, &dest_addr)) {
		return -ENOSPC;
	}
	
	struct jgfs_dir_ent new_ent, *created_ent;
	memset(&new_ent, 0, sizeof(new_ent));
	strlcpy(new_ent.name, name, JGFS_NAME_LIMIT + 1);
	new_ent.type  = TYPE_DIR;
	new_ent.attr  = ATTR_NONE;
	new_ent.mtime = time(NULL);
	new_ent.size  = jgfs_clust_size();
	new_ent.begin = FAT_NALLOC;
	
	int rtn;
	if ((rtn = jgfs_create_ent(parent, &new_ent, &created_ent)) != 0) {
		return rtn;
	}
	
	created_ent->begin = dest_addr;
	
	struct jgfs_dir_clust *dir_clust = jgfs_get_clust(dest_addr);
	jgfs_dir_init(dir_clust);
	
	jgfs_fat_write(dest_addr, FAT_EOF);
	
	return 0;
}

int jgfs_create_symlink(struct jgfs_dir_clust *parent, const char *name,
	const char *target) {
	if (strlen(name) > JGFS_NAME_LIMIT ||
		strlen(target) > jgfs_clust_size() - 1) {
		return -ENAMETOOLONG;
	}
	
	fat_ent_t dest_addr;
	/* make sure a free cluster exists before we bother adding a dir ent to the
	 * parent directory */
	if (!jgfs_fat_find(FAT_FREE, &dest_addr)) {
		return -ENOSPC;
	}
	
	struct jgfs_dir_ent new_ent, *created_ent;
	memset(&new_ent, 0, sizeof(new_ent));
	strlcpy(new_ent.name, name, JGFS_NAME_LIMIT + 1);
	new_ent.type  = TYPE_SYMLINK;
	new_ent.attr  = ATTR_NONE;
	new_ent.mtime = time(NULL);
	new_ent.size  = strlen(target);
	new_ent.begin = FAT_NALLOC;
	
	int rtn;
	if ((rtn = jgfs_create_ent(parent, &new_ent, &created_ent)) != 0) {
		return rtn;
	}
	
	created_ent->begin = dest_addr;
	
	char *symlink_clust = jgfs_get_clust(dest_addr);
	strlcpy(symlink_clust, target, jgfs_clust_size());
	
	jgfs_fat_write(dest_addr, FAT_EOF);
	
	return 0;
}

int jgfs_move_ent(struct jgfs_dir_ent *dir_ent,
	struct jgfs_dir_clust *new_parent) {
	struct jgfs_dir_ent *extant_ent, *new_ent;
	int rtn = jgfs_lookup_child(dir_ent->name, new_parent, &extant_ent);
	if (rtn == 0) {
		if (dir_ent->type == TYPE_DIR) {
			/* only succeed if the target is also a dir and is empty */
			if (extant_ent->type == TYPE_DIR) {
				struct jgfs_dir_clust *extant_dir =
					jgfs_get_clust(extant_ent->begin);
				if (jgfs_dir_count(extant_dir) == 0) {
					jgfs_delete_ent(extant_ent, true);
					new_ent = extant_ent;
				} else {
					return -ENOTEMPTY;
				}
			} else {
				return -EEXIST;
			}
		} else {
			/* can't overwrite a dir with a file */
			if (extant_ent->type == TYPE_DIR) {
				return -EISDIR;
			}
			
			/* overwrite existing files */
			new_ent = extant_ent;
		}
	} else if (rtn == -ENOENT) {
		if ((rtn = jgfs_create_ent(new_parent, dir_ent, &new_ent)) != 0) {
			return rtn;
		}
	} else {
		return rtn;
	}
	
	/* copy the dir ent */
	memcpy(new_ent, dir_ent, sizeof(*new_ent));
	
	/* clear out the old dir ent */
	memset(dir_ent, 0, sizeof(*dir_ent));
	
	return 0;
}

int jgfs_delete_ent(struct jgfs_dir_ent *dir_ent, bool dealloc) {
	if (dealloc) {
		/* check for directory emptiness, if appropriate */
		if (dir_ent->type == TYPE_DIR) {
			struct jgfs_dir_clust *dir_clust = jgfs_get_clust(dir_ent->begin);
			if (jgfs_dir_count(dir_clust) != 0) {
				return -ENOTEMPTY;
			}
			
			/* deallocate the directory cluster */
			jgfs_fat_write(dir_ent->begin, FAT_FREE);
		} else {
			/* deallocate all the clusters associated with the dir ent */
			if (dir_ent->size != 0) {
				jgfs_reduce(dir_ent, 0);
			}
		}
	}
	
	/* erase this dir ent from the parent dir cluster */
	memset(dir_ent, 0, sizeof(*dir_ent));
	
	return 0;
}

uint16_t jgfs_block_count(struct jgfs_dir_ent *dir_ent) {
	fat_ent_t data_addr = dir_ent->begin;
	
	if (dir_ent->size != 0) {
		uint32_t count = 0;
		
		while (data_addr != FAT_EOF) {
			data_addr = jgfs_fat_read(data_addr);
			++count;
		}
		
		return count;
	} else {
		return 0;
	}
}

void jgfs_reduce(struct jgfs_dir_ent *dir_ent, uint32_t new_size) {
	if (new_size >= dir_ent->size) {
		errx(1, "jgfs_reduce: new_size is not smaller");
	}
	
	uint16_t clust_before = CEIL(dir_ent->size, jgfs_clust_size()),
		clust_after = CEIL(new_size, jgfs_clust_size());
	
	if (clust_before != clust_after) {
		fat_ent_t this = dir_ent->begin, next;
		
		for (uint16_t i = 1; i <= clust_before; ++i) {
			next = jgfs_fat_read(this);
			
			/* this means the filesystem is inconsistent */
			if (this == FAT_EOF && i < clust_before) {
				warnx("jgfs_reduce: found premature FAT_EOF in clust chain");
				break;
			}
			
			if (i == clust_after) {
				jgfs_fat_write(this, FAT_EOF);
			} else if (i > clust_after) {
				jgfs_fat_write(this, FAT_FREE);
			}
			
			this = next;
		}
		
		/* special case for zero-size files */
		if (clust_after == 0) {
			jgfs_fat_write(dir_ent->begin, FAT_FREE);
			dir_ent->begin = FAT_NALLOC;
		}
	}
	
	dir_ent->size = new_size;
}

bool jgfs_enlarge(struct jgfs_dir_ent *dir_ent, uint32_t new_size) {
	uint32_t clust_size = jgfs_clust_size();
	
	if (new_size <= dir_ent->size) {
		errx(1, "jgfs_enlarge: new_size is not larger");
	}
	
	bool nospc = false;
	uint16_t clust_before = CEIL(dir_ent->size, clust_size),
		clust_after = CEIL(new_size, clust_size);
	fat_ent_t new_addr;
	
	if (clust_before != clust_after) {
		/* special case for zero-size files */
		if (dir_ent->size == 0) {
			if (jgfs_fat_find(FAT_FREE, &new_addr)) {
				dir_ent->begin = new_addr;
				jgfs_fat_write(new_addr, FAT_EOF);
				
				clust_before = 1;
			} else {
				return false;
			}
		}
		
		fat_ent_t this = dir_ent->begin, next;
		
		for (uint16_t i = 1; i < clust_after; ++i) {
			next = jgfs_fat_read(this);
			
			/* this means the filesystem is inconsistent */
			if (this == FAT_EOF && i < clust_before) {
				warnx("jgfs_enlarge: found premature FAT_EOF in clust chain");
				clust_before = i;
			}
			
			if (i >= clust_before) {
				if (jgfs_fat_find(FAT_FREE, &new_addr)) {
					jgfs_fat_write(this, new_addr);
					jgfs_fat_write(new_addr, FAT_EOF);
					
					this = new_addr;
				} else {
					clust_after = i - 1;
					new_size = clust_after * clust_size;
					nospc = true;
					break;
				}
			} else {
				this = next;
			}
		}
	}
	
	jgfs_zero_span(dir_ent, dir_ent->size, new_size - dir_ent->size);
	
	dir_ent->size = new_size;
	
	return !nospc;
}

void jgfs_zero_span(struct jgfs_dir_ent *dir_ent, uint32_t off, uint32_t size) {
	uint32_t clust_size = jgfs_clust_size();
	
	/* skip to the first cluster to be zeroed */
	fat_ent_t zero_addr = dir_ent->begin;
	while (off >= clust_size) {
		zero_addr = jgfs_fat_read(zero_addr);
		off      -= clust_size;
	}
	
	while (size > 0) {
		uint32_t size_this_cluster;
		
		if (size > (clust_size - off)) {
			size_this_cluster = clust_size - off;
		} else {
			size_this_cluster = size;
		}
		
		struct clust *data_clust = jgfs_get_clust(zero_addr);
		memset((char *)data_clust + off, 0, size_this_cluster);
		
		size -= size_this_cluster;
		off   = 0;
		
		/* next cluster */
		zero_addr = jgfs_fat_read(zero_addr);
	}
}
