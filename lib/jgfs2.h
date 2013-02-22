#ifndef JGFS2_LIB_JGFS2_H
#define JGFS2_LIB_JGFS2_H


#include "macro.h"


#define JGFS2_SECT_SIZE   0x200

#define JGFS2_VER_EXPAND(_maj, _min) \
	(((uint16_t)(_maj) * 0x100) + (uint16_t)(_min))

#define JGFS2_VER_MAJOR   0x00
#define JGFS2_VER_MINOR   0x01
#define JGFS2_VER_TOTAL \
	JGFS2_VER_EXPAND(JGFS2_VER_MAJOR, JGFS2_VER_MINOR)

#define JGFS2_MAGIC       "JGF2"

#define JGFS2_VBR_SECT    0
#define JGFS2_SBLK_SECT   1
#define JGFS2_BOOT_SECT   2

#define JGFS2_LABEL_LIMIT 63
#define JGFS2_NAME_LIMIT  255


/*enum jgfs2_mode {
	JGFS2_S_IFDIR = ...,
	
	JGFS2_S_IXOTH = 0x0001,
};*/

/*enum jgfs2_attr {
	JGFS2_A_NONE = 0,
};*/


struct __attribute__((__packed__)) jgfs2_sect {
	uint8_t data[JGFS2_SECT_SIZE];
};

struct __attribute__((__packed__)) jgfs2_inode {
	uint32_t i_attr;  // file attributes
	uint16_t i_mode;  // type and permissions
	
	uint16_t i_uid;   // owner
	uint16_t i_gid;   // group
	
	uint32_t i_size;  // length in bytes of file content
	
	int64_t  i_atime; // access time
	int64_t  i_ctime; // creation time
	int64_t  i_mtime; // modification time
	
	uint32_t i_nlink; // hard link ref count
	
	uint32_t i_gen;   // generation
	
	/* TODO: extent info */
};

struct __attribute__((__packed__)) jgfs2_dentry {
	uint32_t d_inode;    // inode number (zero if last in list)
	
	uint8_t  d_name_len; // length of name (without NULL)
	char     d_name[0];  // null-terminated name
};

struct __attribute__((__packed__)) jgfs2_directory {
	struct jgfs2_dentry dents[0];
};

struct __attribute__((__packed__)) jgfs2_superblock {
	char     s_magic[4];       // must be "JGF2"
	
	uint8_t  s_ver_major;      // major version
	uint8_t  s_ver_minor;      // minor version
	
	uint64_t s_sect_count;     // total number of sectors
	uint16_t s_boot_size;      // number of boot sectors
	
	uint16_t s_block_size;     // sectors per block
	
	int64_t  s_ctime;          // fs creation time
	int64_t  s_mtime;          // fs last mount time
	
	uint8_t  s_uuid[16];       // fs uuid
	
	char     s_label[JGFS2_LABEL_LIMIT + 1]; // null-terminated volume label
	
	uint32_t s_addr_fs_bitmap; // address of free space bitmap
	uint32_t s_addr_root_dir;  // address of root directory
	
	char     s_rsvd[0x186];
};

struct jgfs2_mkfs_param {
	uint8_t  uuid[16];   // fs uuid; must be initialized
	
	char     label[JGFS2_LABEL_LIMIT + 1]; // null-terminated volume label
	
	uint64_t sect_count; // total number of sectors; zero: fill device
	uint16_t boot_size;  // number of boot sectors
	
	uint16_t block_size; // sectors per block; zero: auto-select
	
	bool     zap_vbr;    // true: zero the volume boot record
	bool     zap_boot;   // true: zero the boot area
	bool     zap_data;   // true: zero the entire data area
};

struct jgfs2_mount_options {
	bool read_only; // disallow write operations
};


_Static_assert(sizeof(struct jgfs2_sect) == 0x200,
	"struct jgfs2_sect must be 512 bytes");
_Static_assert(sizeof(struct jgfs2_superblock) == 0x200,
	"struct jgfs2_superblock must be 512 bytes");


#if 0
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


#endif
