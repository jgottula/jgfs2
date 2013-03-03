/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#ifndef JGFS2_LIB_TREE_NODE_H
#define JGFS2_LIB_TREE_NODE_H


#include "../jgfs2.h"
#include "../fs.h"
#include "key.h"
#include "item.h"


#define ASSERT_ROOT(_node_addr) \
	if (!node_is_root(_node_addr)) { \
		errx("%s: not root: node 0x%" PRIx32, \
			__func__, _node_addr); \
	}
#define ASSERT_BRANCH(_node_ptr) \
	if (_node_ptr->hdr.leaf) { \
		errx("%s: not branch: node 0x%" PRIx32, \
			__func__, _node_ptr->hdr.this); \
	}
#define ASSERT_LEAF(_node_ptr) \
	if (!_node_ptr->hdr.leaf) { \
		errx("%s: not leaf: node 0x%" PRIx32, \
			__func__, _node_ptr->hdr.this); \
	}
#define ASSERT_NONEMPTY(_node_ptr) \
	if (_node_ptr->hdr.cnt == 0) { \
		errx("%s: empty node: node 0x%" PRIx32, \
			__func__, _node_ptr->hdr.this); \
	}


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
	uint32_t prev;
	uint32_t next;
	uint32_t parent;
};

struct __attribute__((__packed__)) tree_node {
	struct node_hdr hdr;
};

struct __attribute__((__packed__)) branch_node {
	struct node_hdr hdr;
	node_ref elems[0];
};

struct __attribute__((__packed__)) leaf_node {
	struct node_hdr hdr;
	item_ref elems[0];
};


typedef struct tree_node   *node_ptr;
typedef struct branch_node *branch_ptr;
typedef struct leaf_node   *leaf_ptr;


static uint32_t node_size_blk(void) {
	return 1;
}

static uint32_t node_size_byte(void) {
	return node_size_blk() * fs.blk_size;
}

static uint32_t node_size_usable(void) {
	return node_size_byte() - sizeof(struct node_hdr);
}


/* generic node functions */
uint32_t node_alloc(void);

node_ptr node_map(uint32_t node_addr);
void node_unmap(const node_ptr node);

void node_dump(uint32_t node_addr, bool recurse);

uint32_t node_used(const node_ptr node);
uint32_t node_free(const node_ptr node);

bool node_is_root(uint32_t node_addr);
uint32_t node_find_root(uint32_t node_addr);

const key *node_key(const node_ptr node, uint16_t idx);
const key *node_first_key(const node_ptr node);

void *node_search(const node_ptr node, const key *key);
uint16_t node_search_hypo(const node_ptr node, const key *key);

void node_zero_data(node_ptr node);
void node_copy_data(node_ptr dst, const node_ptr src);

void node_split(uint32_t node_addr);


/* branch node functions */
branch_ptr branch_init(uint32_t node_addr, uint32_t parent);

void branch_dump(const branch_ptr node);

uint32_t branch_used(const branch_ptr node);
uint32_t branch_free(const branch_ptr node);

uint16_t branch_half(const branch_ptr node);

node_ref *branch_search_addr(const branch_ptr node, uint32_t addr);

void branch_zero(branch_ptr node, uint16_t first);
void branch_xfer_half(branch_ptr dst, branch_ptr src);

void branch_paternalize(branch_ptr node);

void branch_append_naive(branch_ptr node, const node_ref *elem);
bool branch_insert(branch_ptr node, const node_ref *elem);

void branch_ref(branch_ptr node, node_ptr child);
void branch_ref_update(branch_ptr node, node_ptr child);

void branch_split_post(branch_ptr this, branch_ptr new, bool was_root);


/* leaf node functions */
leaf_ptr leaf_init(uint32_t node_addr, uint32_t parent, uint32_t prev,
	uint32_t next);

void leaf_dump(const leaf_ptr node);

uint32_t leaf_used(const leaf_ptr node);
uint32_t leaf_free(const leaf_ptr node);

uint16_t leaf_half(const leaf_ptr node);

void *leaf_data_ptr(const leaf_ptr node, const item_ref *item);

void leaf_zero(leaf_ptr node, uint16_t first);
void leaf_xfer_half(leaf_ptr dst, leaf_ptr src);

void leaf_insert_naive(leaf_ptr node, uint16_t at, const key *key,
	struct item_data item);
void leaf_append_naive(leaf_ptr node, const key *key, struct item_data item);
bool leaf_insert(leaf_ptr node, const key *key, struct item_data item);

void leaf_split_post(leaf_ptr this, leaf_ptr new);


#endif
