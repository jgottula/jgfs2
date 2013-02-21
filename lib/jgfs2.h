#ifndef JGFS2_LIB_JGFS2_H
#define JGFS2_LIB_JGFS2_H


#include "macro.h"


#define SECT_SIZE 0x200

#define JGFS_VER_MAJOR 0x04
#define JGFS_VER_MINOR 0x01
#define JGFS_VER_TOTAL 0x0401

#define JGFS_MAGIC "JGFS"

#define JGFS_VBR_SECT  0
#define JGFS_HDR_SECT  1
#define JGFS_BOOT_SECT 2

#define JGFS_NAME_LIMIT  19
#define JGFS_LABEL_LIMIT 19

#define JGFS_FENT_PER_S \
	(SECT_SIZE / sizeof(fat_ent_t))

#define JGFS_DENT_PER_C \
	(jgfs_clust_size() / sizeof(struct jgfs_dir_ent))

#define JGFS_MAX_FAT_SECT \
	CEIL(0x10000 / JGFS_FENT_PER_S)

#define JGFS_VER_EXPAND(_maj, _min) \
	(((uint16_t)_maj * 0x100) + (uint16_t)_min)


typedef uint16_t fat_ent_t;


enum jgfs_fat_val {
	FAT_FREE   = 0x0000, // free
	FAT_ROOT   = 0x0000, // root directory
	
	FAT_FIRST  = 0x0001, // first normal cluster
	/* ... */
	FAT_LAST   = 0xfffb, // last possible normal cluster
	
	/* special */
	FAT_EOF    = 0xfffc, // last cluster in file
	FAT_RSVD   = 0xfffd, // reserved
	FAT_BAD    = 0xfffe, // damaged
	FAT_OOB    = 0xffff, // past end of device
	FAT_NALLOC = 0xffff, // file not allocated
};

enum jgfs_file_type {
	TYPE_FILE    = (1 << 0), // regular file
	TYPE_DIR     = (1 << 1), // directory
	TYPE_SYMLINK = (1 << 2), // symlink
};

enum jgfs_file_attr {
	ATTR_NONE = 0,
	
	/* ... */
};


struct sect {
	uint8_t data[SECT_SIZE];
};

struct jgfs_fat_sect {
	fat_ent_t entries[JGFS_FENT_PER_S];
};

struct  __attribute__((__packed__)) jgfs_dir_ent {
	char      name[JGFS_NAME_LIMIT + 1]; // [A-Za-z0-9_.] zero padded
	                                     // empty string means unused entry
	uint8_t   type;     // type (mutually exclusive)
	uint8_t   attr;     // attributes (bitmask)
	uint32_t  mtime;    // unix time
	uint32_t  size;     // size in bytes
	fat_ent_t begin;    // first cluster (FAT_NALLOC for empty file)
};

struct __attribute__((__packed__)) jgfs_dir_clust {
	struct jgfs_dir_ent entries[0];
};

/* this header must be located at sector 1 (offset 0x200~0x400) */
struct __attribute__((__packed__)) jgfs_hdr {
	char     magic[4];  // must be "JGFS"
	uint8_t  ver_major; // major version
	uint8_t  ver_minor; // minor version
	
	uint32_t s_total;   // total number of sectors
	
	uint16_t s_boot;    // sectors reserved for the boot area
	uint16_t s_fat;     // sectors reserved for the fat
	
	uint16_t s_per_c;   // sectors per cluster
	
	uint32_t ctime;     // fs creation time
	uint32_t mtime;     // fs last mount time
	
	char label[JGFS_LABEL_LIMIT + 1];
	
	struct jgfs_dir_ent root_dir_ent; // root directory entry
	
	char     reserved[0x1b4];
};

struct jgfs_mkfs_param {
	char label[JGFS_LABEL_LIMIT + 1];
	
	uint32_t s_total; // set to zero to fill the device
	uint16_t s_boot;
	uint16_t s_per_c; // set to zero to auto-choose the best value
	
	bool zero_data;   // set to true to zero all data clusters
	bool zap;         // set to true to zero the vbr and boot area
};

struct jgfs {
	struct jgfs_hdr      *hdr;
	struct sect          *boot;
	struct jgfs_fat_sect *fat;
};


_Static_assert(sizeof(struct sect) == 0x200,
	"sect must be 512 bytes");
_Static_assert(sizeof(struct jgfs_hdr) == 0x200,
	"jgfs_hdr must be 512 bytes");
_Static_assert(sizeof(struct jgfs_fat_sect) == 0x200,
	"jgfs_fat_sect must be 512 bytes");
_Static_assert(512 % sizeof(struct jgfs_dir_ent) == 0,
	"jgfs_dir_ent must go evenly into 512 bytes");


typedef int (*jgfs_dir_func_t)(struct jgfs_dir_ent *, void *);


/* load jgfs from the device at dev_path */
void jgfs_init(const char *dev_path);
/* make new jgfs on the device at dev_path with the given parameters */
void jgfs_new(const char *dev_path, struct jgfs_mkfs_param *param);
/* sync and close the filesystem */
void jgfs_done(void);
/* sync the filesystem to disk */
void jgfs_sync(void);

/* get the cluster size (in bytes) of the loaded filesystem */
uint32_t jgfs_clust_size(void);
/* get the number of clusters in the loaded filesystem */
uint16_t jgfs_fs_clusters(void);
/* get a pointer to a sector */
void *jgfs_get_sect(uint32_t sect_num);
/* get a pointer to a cluster */
void *jgfs_get_clust(fat_ent_t clust_num);

/* read the fat entry at addr */
fat_ent_t jgfs_fat_read(fat_ent_t addr);
/* write val to the fat entry at addr */
void jgfs_fat_write(fat_ent_t addr, fat_ent_t val);
/* get the address of the first cluster with the target value in the fat, or
 * return false on failure to find one */
bool jgfs_fat_find(fat_ent_t target, fat_ent_t *first);
/* count fat entries with the target value (use FAT_FREE for free blocks) */
uint16_t jgfs_fat_count(fat_ent_t target);
/* dump the entire fat to stderr */
void jgfs_fat_dump(void);

/* find dir clust corresponding to the second-to-last path component, plus the
 * dir ent corresponding to the last component (or NULL for just the parent);
 * return posix error code on failure */
int jgfs_lookup(const char *path, struct jgfs_dir_clust **parent,
	struct jgfs_dir_ent **child);
/* find child with name in parent; return posix error code on failure */
int jgfs_lookup_child(const char *name, struct jgfs_dir_clust *parent,
	struct jgfs_dir_ent **child);

/* initialize (zero out) a dir cluster with no entries */
void jgfs_dir_init(struct jgfs_dir_clust *dir_clust);
/* count the number of dir ents in the given directory */
uint32_t jgfs_dir_count(struct jgfs_dir_clust *dir_clust);
/* call func once for each dir ent in parent with the dir ent and the
 * user-provided pointer as arguments; if func returns nonzero, the foreach
 * immediately terminates with the same return value */
int jgfs_dir_foreach(jgfs_dir_func_t func, struct jgfs_dir_clust *dir_clust,
	void *user_ptr);

/* add new (valid) dir ent to parent and return a pointer to it as created_ent
 * (if not NULL); return posix error code on failure */
int jgfs_create_ent(struct jgfs_dir_clust *parent,
	const struct jgfs_dir_ent *new_ent, struct jgfs_dir_ent **created_ent);
/* add new file called name to parent; return posix error code on failure */
int jgfs_create_file(struct jgfs_dir_clust *parent, const char *name);
/* add new dir called name to parent; return posix error code on failure */
int jgfs_create_dir(struct jgfs_dir_clust *parent, const char *name);
/* add new symlink called name with target to parent; return posix error code on
 * failure */
int jgfs_create_symlink(struct jgfs_dir_clust *parent, const char *name,
	const char *target);

/* transplant dir_ent from its current parent to new_parent */
int jgfs_move_ent(struct jgfs_dir_ent *dir_ent,
	struct jgfs_dir_clust *new_parent);
/* delete the given dir ent from parent, deallocating the file or directory if
 * requested; return posix error code on failure */
int jgfs_delete_ent(struct jgfs_dir_ent *dir_ent, bool dealloc);

/* count the clusters taken up by a file or directory */
uint16_t jgfs_block_count(struct jgfs_dir_ent *dir_ent);

/* reduce the size of a file */
void jgfs_reduce(struct jgfs_dir_ent *dir_ent, uint32_t new_size);
/* increase the size of a file; returns false on insufficient space */
bool jgfs_enlarge(struct jgfs_dir_ent *dir_ent, uint32_t new_size);

/* fill a span of the given dir ent's data clusters with zeroes */
void jgfs_zero_span(struct jgfs_dir_ent *dir_ent, uint32_t off, uint32_t size);


extern struct jgfs jgfs;


#endif
