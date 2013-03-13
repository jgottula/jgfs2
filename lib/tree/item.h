/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#ifndef JGFS2_LIB_TREE_ITEM_H
#define JGFS2_LIB_TREE_ITEM_H


#include "../jgfs2.h"
#include "key.h"


enum item_key {
	KEY_INODE = 0x01,
};


struct __attribute__((__packed__)) inode_item {
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
};


#endif
