/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "node.h"
#include "../debug.h"
#include "../extent.h"


uint32_t node_alloc(void) {
	/* allocating a block should be okay because the ext tree does not insert
	 * items on allocations; otherwise, we could deadlock, since extent
	 * allocation depends on a tree and tree insertion can depend on extent
	 * allocation */
	return ext_alloc(node_size_blk());
}

node_ptr node_map(uint32_t node_addr) {
	return (node_ptr)fs_map_blk(node_addr, node_size_blk());
}

void node_unmap(const node_ptr node) {
	const struct node_hdr *hdr = (const struct node_hdr *)node;
	
	fs_unmap_blk(node, hdr->this, node_size_blk());
}

void node_dump(uint32_t node_addr) {
	struct node_hdr *hdr = fs_map_blk(node_addr, node_size_blk());
	
	if (hdr->leaf) {
		leaf_dump((leaf_ptr)hdr);
	} else {
		branch_dump((branch_ptr)hdr);
	}
	
	fs_unmap_blk(hdr, node_addr, node_size_blk());
}

bool node_is_root(uint32_t node_addr) {
	const node_ptr node = node_map(node_addr);
	bool is_root = (node->hdr.parent == 0);
	node_unmap(node);
	
	return is_root;
}

uint32_t node_find_root(uint32_t node_addr) {
	uint32_t parent_addr = node_addr;
	
	do {
		node_addr = parent_addr;
		
		node_ptr node = node_map(node_addr);
		parent_addr = node->hdr.parent;
		node_unmap(node);
	} while (parent_addr != 0);
	
	return node_addr;
}

const key *node_first_key(const node_ptr node) {
	ASSERT_NONEMPTY(node);
	
	if (node->hdr.leaf) {
		return &((leaf_ptr)node)->elems[0].key;
	} else {
		return &((branch_ptr)node)->elems[0].key;
	}
}

void node_zero_data(node_ptr node) {
	uint8_t *zero_ptr = (uint8_t *)node + sizeof(struct node_hdr);
	size_t   zero_len = node_size_usable();
	
	memset(zero_ptr, 0, zero_len);
}

void node_copy_data(node_ptr dst, const node_ptr src) {
	uint8_t       *copy_dst = (uint8_t *)dst + sizeof(struct node_hdr);
	const uint8_t *copy_src = (const uint8_t *)src + sizeof(struct node_hdr);
	size_t         copy_len = node_size_usable();
	
	memcpy(copy_dst, copy_src, copy_len);
}

void node_split(uint32_t this_addr) {
	node_ptr this = node_map(this_addr);
	
	uint32_t parent_addr = this->hdr.parent;
	branch_ptr parent;
	
	uint32_t new_addr = node_alloc();
	node_ptr new;
	
	bool was_root = node_is_root(this_addr);
	
	/* if we are root, allocate *two* new nodes to be children of root, leaving
	 * this as the new root, so the addr of the tree's root doesn't change;
	 * if we are not the root node, try to allocate a node_ref in the parent
	 * node, and if that fails, recurse!
	 */
	if (was_root) {
		parent_addr = this_addr;
		parent      = (branch_ptr)this;
		
		this_addr = node_alloc();
		
		if (this->hdr.leaf) {
			this = (node_ptr)leaf_init(this_addr, parent_addr,
				parent->hdr.prev, new_addr);
		} else {
			this = (node_ptr)branch_init(this_addr, parent_addr);
		}
		
		node_copy_data(this, (node_ptr)parent);
		this->hdr.cnt = parent->hdr.cnt;
		
		parent->hdr.parent = 0;
		parent->hdr.prev   = 0;
		parent->hdr.next   = 0;
		parent->hdr.cnt    = 0;
		parent->hdr.leaf   = false;
		
		node_zero_data((node_ptr)parent);
	} else {
		/* we handle possible parent splits first so that the recursion will
		 * go cleanly, without worry for partial changes down at this level */
		parent = (branch_ptr)node_map(parent_addr);
		if (branch_free(parent) < sizeof(node_ref)) {
			/* remap the parent in case it was moved in the split */
			node_unmap((node_ptr)parent);
			node_split(parent_addr);
			parent = (branch_ptr)node_map(parent_addr);
		}
	}
	
	if (this->hdr.leaf) {
		new = (node_ptr)leaf_init(new_addr, parent_addr, this_addr,
			this->hdr.next);
		leaf_split_post((leaf_ptr)this, (leaf_ptr)new);
	} else {
		new = (node_ptr)branch_init(new_addr, parent_addr);
		branch_split_post((branch_ptr)this, (branch_ptr)new, was_root);
	}
	
	if (was_root) {
		branch_ref(parent, this);
	}
	branch_ref(parent, new);
	
	node_unmap((node_ptr)parent);
	node_unmap(new);
	node_unmap(this);
}
