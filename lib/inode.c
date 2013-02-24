#include "inode.h"
#include "fs.h"


struct jgfs2_inode *jgfs2_inode_get(uint32_t inode_num) {
	return (struct jgfs2_inode *)fs.inode_table + inode_num;
}
