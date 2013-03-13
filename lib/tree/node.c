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

node_ptr node_map(uint32_t node_addr, bool writable) {
	return (node_ptr)fs_map_blk(node_addr, node_size_blk(), writable);
}

void node_unmap(const node_ptr node) {
	const struct node_hdr *hdr = (const struct node_hdr *)node;
	
	fs_msync_blk(node, hdr->this, node_size_blk(), true);
	fs_unmap_blk(node, hdr->this, node_size_blk());
}

void node_dump(uint32_t node_addr, bool recurse) {
	node_ptr node = node_map(node_addr, false);
	
	if (node->hdr.leaf) {
		leaf_dump((leaf_ptr)node);
	} else {
		branch_ptr branch = (branch_ptr)node;
		branch_dump(branch);
		
		if (recurse) {
			const node_ref *elem_end = branch->elems + branch->hdr.cnt;
			for (const node_ref *elem = branch->elems;
				elem < elem_end; ++elem) {
				node_dump(elem->addr, true);
			}
		}
	}
	
	node_unmap(node);
}

uint32_t node_used(uint32_t node_addr) {
	const node_ptr node = node_map(node_addr, false);
	
	uint32_t used;
	if (node->hdr.leaf) {
		used = leaf_used((const leaf_ptr)node);
	} else {
		used = branch_used((const branch_ptr)node);
	}
	
	node_unmap(node);
	
	return used;
}

uint32_t node_free(uint32_t node_addr) {
	const node_ptr node = node_map(node_addr, false);
	
	uint32_t free;
	if (node->hdr.leaf) {
		free = leaf_free((const leaf_ptr)node);
	} else {
		free = branch_free((const branch_ptr)node);
	}
	
	node_unmap(node);
	
	return free;
}

bool node_is_root(uint32_t node_addr) {
	const node_ptr node = node_map(node_addr, false);
	bool is_root = (node->hdr.parent == 0);
	node_unmap(node);
	
	return is_root;
}

uint32_t node_find_root(uint32_t node_addr) {
	uint32_t parent_addr = node_addr;
	
	do {
		node_addr = parent_addr;
		
		node_ptr node = node_map(node_addr, false);
		parent_addr = node->hdr.parent;
		node_unmap(node);
	} while (parent_addr != 0);
	
	return node_addr;
}

const key *node_key(const node_ptr node, uint16_t idx) {
	if (idx >= node->hdr.cnt) {
		errx("%s: idx exceeds bounds: node 0x%" PRIx32 ", %" PRIu16
			" >= %" PRIu16, __func__, node->hdr.this, idx, node->hdr.cnt);
	}
	
	if (node->hdr.leaf) {
		return &((leaf_ptr)node)->elems[idx].key;
	} else {
		return &((branch_ptr)node)->elems[idx].key;
	}
}

const key *node_first_key(const node_ptr node) {
	ASSERT_NONEMPTY(node);
	
	return node_key(node, 0);
}

bool node_search(const node_ptr node, const key *key, uint16_t *out) {
	/* circumvent unsigned wraparound if node->hdr.cnt == 0 */
	if (node->hdr.cnt == 0) {
		return false;
	}
	
	uint16_t first = 0;
	uint16_t last  = node->hdr.cnt - 1;
	uint16_t middle;
	
	/* goal: find key that == wanted key */
	while (first <= last) {
		middle = (first + last) / 2;
		
		int8_t cmp = key_cmp(node_key(node, middle), key);
		
		if (cmp < 0) {
			first = middle + 1;
		} else if (cmp > 0) {
			last = middle - 1;
		} else {
			/* found */
			*out = middle;
			return true;
		}
	}
	
	/* not found */
	return false;
}

uint16_t node_search_hypo(const node_ptr node, const key *key) {
	/* for empty node, return first index; for largest key, return very last
	 * possible index */
	if (node->hdr.cnt == 0) {
		return 0;
	} else if (key_cmp(node_key(node, node->hdr.cnt - 1), key) < 0) {
		return node->hdr.cnt;
	}
	
	uint16_t first = 0;
	uint16_t last  = node->hdr.cnt - 1;
	uint16_t middle;
	
	/* goal: find lowest key that is > wanted key */
	while (first <= last) {
		middle = CEIL(first + last, 2);
		
		int8_t cmp = key_cmp(node_key(node, middle), key);
		
		if (cmp < 0) {
			first = middle + 1;
		} else if (cmp > 0) {
			if (middle == 0 || key_cmp(node_key(node, middle - 1), key) < 0) {
				return middle;
			} else {
				last = middle - 1;
			}
		} else {
			errx("%s: key already present: node 0x%" PRIx32 " key %s",
				__func__, node->hdr.this, key_str(key));
		}
	}
	
	errx("%s: total failure: node 0x%" PRIx32 " key %s",
		__func__, node->hdr.this, key_str(key));
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
	node_ptr this = node_map(this_addr, true);
	
	uint32_t parent_addr = this->hdr.parent;
	branch_ptr parent = NULL;
	
	uint32_t new_addr = node_alloc();
	node_ptr new = NULL;
	
	bool was_root = node_is_root(this_addr);
	
	/* if we are root, allocate *two* new nodes to be children of root, leaving
	 * this as the new root, so the addr of the tree's root doesn't change;
	 * if we are not the root node, try to allocate a node_ref in the parent
	 * node, and if that fails, recurse!
	 */
	if (was_root) {
		parent_addr = this_addr;
		parent = (branch_ptr)this;
		
		this_addr = node_alloc();
		
		if (this->hdr.leaf) {
			this = (node_ptr)leaf_init(this_addr, parent_addr, 0, 0);
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
		/* we handle splits in a depth-first manner: we recurse to the parent,
		 * if necessary, *before* we split this node */
		parent = (branch_ptr)node_map(parent_addr, true);
		if (branch_free(parent) < sizeof(node_ref)) {
			node_unmap((node_ptr)parent);
			node_split(parent_addr);
			
			/* remap the parent, in case it changed */
			parent_addr = this->hdr.parent;
			parent = (branch_ptr)node_map(parent_addr, true);
		}
	}
	
	if (this->hdr.leaf) {
		new = (node_ptr)leaf_init(new_addr, parent_addr, this_addr,
			this->hdr.next);
		this->hdr.next = new_addr;
		
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
