/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#ifndef JGFS2_LIB_TREE_NODE_H
#define JGFS2_LIB_TREE_NODE_H


#include "../fs.h"
#include "key.h"
#include "item.h"


#define ASSERT_NONEMPTY(_node_ptr) \
	if (_node_ptr->hdr.cnt == 0) { \
		errx("%s: empty node: node 0x%" PRIx32, \
			__func__, _node_ptr->hdr.this); \
	}
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


typedef struct __attribute__((__packed__)) {
	key key;
	uint32_t len;
	uint32_t off;
} item_ref;

typedef struct __attribute__((__packed__)) {
	key key;
	uint32_t addr;
} node_ref;

typedef struct __attribute__((__packed__)) {
	key key;
} elem;

struct __attribute__((__packed__)) node_hdr {
	bool leaf;
	uint16_t cnt;
	uint32_t this;
	uint32_t prev;
	uint32_t next;
	uint32_t parent;
};

struct __attribute__((__packed__)) node {
	struct node_hdr hdr;
	
	union {
		node_ref b_elems[0];
		item_ref l_elems[0];
	};
};

union elem_payload {
	uint32_t b_addr;
	struct {
		uint32_t l_len;
		void *l_data;	
	};
};


typedef struct node *node_ptr;


static uint32_t node_size_blk(void) {
	return 1;
}

static uint32_t node_size_byte(void) {
	return node_size_blk() * fs.blk_size;
}

static uint32_t node_size_usable(void) {
	return node_size_byte() - sizeof(struct node_hdr);
}

static elem *node_elem(const node_ptr node, uint16_t idx) {
	if (node->hdr.leaf) {
		return (elem *)(node->l_elems + idx);
	} else {
		return (elem *)(node->b_elems + idx);
	}
}


#warning take a look at node_max_cnt
/*static uint16_t node_max_cnt(void) {
	if (sizeof(item_ref) > sizeof(node_ref)) {
		return (node_size_usable() / sizeof(item_ref)) + 1;
	} else {
		return (node_size_usable() / sizeof(node_ref)) + 1;
	}
}*/


#warning check all functions for possibility of const pointers
#warning also check all static functions

#warning one-use internal functions should be made static in one code file


/* debugging */
void node_dump(uint32_t node_addr, bool recurse);

/* allocation */
uint32_t node_alloc(void);
void node_dealloc(uint32_t node_addr);

/* mapping */
node_ptr node_map(uint32_t node_addr, bool writable);
void node_unmap(const node_ptr node);

/* initialization */
node_ptr node_init(uint32_t node_addr, bool leaf, uint32_t parent,
	uint32_t prev, uint32_t next);
node_ptr node_copy_init(uint32_t dst_addr, const node_ptr src, bool leaf,
	uint32_t parent, uint32_t prev, uint32_t next);

/* miscellaneous */
bool node_is_root(uint32_t node_addr);
uint32_t node_find_root(uint32_t node_addr);

/* space usage */
uint32_t node_used(const node_ptr node);
uint32_t node_free(const node_ptr node);

/* keys */
key *node_key(const node_ptr node, uint16_t idx);
key *node_first_key(const node_ptr node);

/* elements */
void *leaf_elem_data(const node_ptr leaf, uint16_t idx);

/* bulk operations */
void node_zero_all(node_ptr node);
void node_zero_range(node_ptr node, uint16_t first);
void node_shift_forward(node_ptr node, uint16_t first, uint16_t diff_elem,
	uint32_t diff_data);
void node_shift_backward(node_ptr node, uint16_t first, uint16_t diff_elem,
	uint32_t diff_data);
void node_xfer(node_ptr dst, const node_ptr src, uint16_t dst_idx,
	uint16_t src_idx, uint16_t cnt);

/* searching */
bool node_search(const node_ptr node, const key *key, uint16_t *out);
uint16_t node_search_hypo(const node_ptr node, const key *key);
uint32_t branch_search(const node_ptr branch, const key *key);
node_ref *branch_search_addr(const node_ptr branch, uint32_t addr);

/* modifying */
bool node_insert(node_ptr node, const key *key,
	const union elem_payload *payload);


#if 0

void node_split(uint32_t this_addr);
void node_merge(uint32_t this_addr);


void branch_append_naive(branch_ptr node, const node_ref *elem);
void branch_insert_naive(branch_ptr node, uint16_t at, const node_ref *elem);
bool branch_insert(branch_ptr node, const node_ref *elem);

bool branch_remove(branch_ptr node, const key *key);

void branch_split_post(branch_ptr this, branch_ptr new, bool was_root);


/* leaf node functions */
void leaf_insert_naive(leaf_ptr node, uint16_t at, const key *key,
	struct item_data item);
void leaf_append_naive(leaf_ptr node, const key *key, struct item_data item);
bool leaf_insert(leaf_ptr node, const key *key, struct item_data item);

bool leaf_remove(leaf_ptr node, const key *key);

void leaf_split_post(leaf_ptr this, leaf_ptr new);
#endif


#endif
