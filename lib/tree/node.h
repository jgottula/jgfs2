#ifndef JGFS2_LIB_TREE_NODE_H
#define JGFS2_LIB_TREE_NODE_H


#include "../jgfs2.h"
#include "../fs.h"
#include "common.h"
#include "key.h"
#include "item.h"


typedef struct __attribute__((__packed__)) {
	key key;
	uint32_t off;
	uint32_t len;
} item_ref;

typedef struct __attribute__((__packed__)) {
	key key;
	uint32_t addr;
} node_ref;

struct __attribute__((__packed__)) node_hdr {
	bool leaf;
	uint16_t cnt;
	uint32_t this;
	uint32_t parent;
	uint32_t next;
};

struct __attribute__((__packed__)) branch_node {
	struct node_hdr hdr;
	node_ref elems[0];
};

struct __attribute__((__packed__)) leaf_node {
	struct node_hdr hdr;
	item_ref elems[0];
};

union tree_node {
	struct branch_node branch;
	struct leaf_node   leaf;
};


typedef union  tree_node   *node_ptr;
typedef struct branch_node *branch_ptr;
typedef struct leaf_node   *leaf_ptr;


void node_dump(uint32_t node_addr);
node_ptr node_map(uint32_t node_addr);
void node_unmap(const node_ptr node);

void branch_dump(const branch_ptr node);
branch_ptr branch_init(uint32_t node_addr, uint32_t parent);
uint32_t branch_used(const branch_ptr node);
uint32_t branch_free(const branch_ptr node);
uint16_t branch_half(const branch_ptr node);
void branch_zero(branch_ptr node, uint16_t first);
void branch_append_naive(branch_ptr node, const node_ref *elem);
void branch_xfer_half(branch_ptr dst, branch_ptr src);

void leaf_dump(const leaf_ptr node);
leaf_ptr leaf_init(uint32_t node_addr, uint32_t parent, uint32_t next);
tree_result leaf_item_ptr(const leaf_ptr node, const key *key);
uint32_t leaf_used(const leaf_ptr node);
uint32_t leaf_free(const leaf_ptr node);
uint16_t leaf_half(const leaf_ptr node);
void leaf_zero(leaf_ptr node, uint16_t first);
void leaf_append_naive(leaf_ptr node, const key *key, uint32_t len,
	const void *data);
void leaf_xfer_half(leaf_ptr dst, leaf_ptr src);


inline uint32_t node_size_blk(void) {
	return 1;
}

inline uint32_t node_size_byte(void) {
	return node_size_blk() * fs.blk_size;
}


#endif
