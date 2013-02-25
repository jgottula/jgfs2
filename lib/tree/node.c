#include "node.h"
#include "../debug.h"
#include "../extent.h"


void node_dump(uint32_t node_addr) {
	struct node_hdr *hdr = fs_map_blk(node_addr, node_size_blk());
	
	if (hdr->leaf) {
		leaf_dump((leaf_ptr)hdr);
	} else {
		branch_dump((branch_ptr)hdr);
	}
	
	fs_unmap_blk(hdr, node_addr, node_size_blk());
}

struct check_result node_check(uint32_t node_addr) {
	// check: sort order
	// check: not empty (ok for root only)
	// check: no dupes
	
	// branch: elem->key == elem->addr->elems[0].key
	
	// leaf: no overlaps
	// leaf: size valid for type
	
	// put branch checks in branch_check
	// put leaf checks in leaf_check
}

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

void node_copy_data(node_ptr dst, const node_ptr src) {
	uint8_t       *copy_dst = (uint8_t *)dst + sizeof(struct node_hdr);
	const uint8_t *copy_src = (const uint8_t *)src + sizeof(struct node_hdr);
	size_t         copy_len = node_size_byte() - sizeof(struct node_hdr);
	
	memcpy(copy_dst, copy_src, copy_len);
}

void node_split(uint32_t node_addr) {
	uint32_t new_addr = node_alloc();
	
	node_ptr this = node_map(node_addr);
	node_ptr new;
	
	bool was_root = (this->hdr.parent == 0);
	
	TODO("test this");
	
	/* if we are root, allocate *two* new nodes to be children of root, leaving
	 * this as the new root, so the addr of the tree's root doesn't change;
	 * if we are not the root node, try to allocate a node_ref in the parent
	 * node, and if that fails, recurse!
	 */
	if (was_root) {
		/*node_ptr root;
		node_ptr this2;
		
		if (this->hdr.leaf) {
			
		} else {
			
		}
		
		node_copy_data(this2, this);
		this2->hdr.parent = node_addr;
		
		root->hdr.parent = 0;*/
		
		
		
		
	/*node->hdr.this   = node_addr;
	node->hdr.parent = parent;
	node->hdr.next   = 0;
	node->hdr.cnt    = 0;
	node->hdr.leaf   = false;*/
	
	/*node->hdr.this   = node_addr;
	node->hdr.parent = parent;
	node->hdr.next   = next;
	node->hdr.cnt    = 0;
	node->hdr.leaf   = true;*/
		
		/* at the end of this, we should be in the same situation as with the
		 * non-root block so that subsequent code need not know about the
		 * shenanigans we had to pull here */
	} else {
		if (this->hdr.leaf) {
			new = (node_ptr)leaf_init(new_addr, this->hdr.parent,
				this->hdr.next);
		} else {
			new = (node_ptr)branch_init(new_addr, this->hdr.parent);
		}
		
		this->hdr.next = new_addr;
	}
	
	if (this->hdr.leaf) {
		leaf_split((leaf_ptr)this, (leaf_ptr)new);
	} else {
		branch_split((branch_ptr)this, (branch_ptr)new, was_root);
	}
}


/* DO NOT delete the block below until all comments/TODOs have been transferred
 * or addressed! */

#if 0
	/* if this is a branch, new's children's parent values need to be updated;
	 * if was_root, then the same needs to done for this's children */
	
	
	/* allocating a block should be okay because the ext tree does not insert
	 * items on allocations; otherwise, we could deadlock */
	uint32_t new_addr = ext_alloc(1);
	struct jgfs2_node *new_node = fs_map_blk(new_addr, 1);
	
	const struct jgfs2_key *first_key, *second_key;
	
	uint16_t split_at = node->hdr.item_qty / 2;
	if (node->hdr.leaf) {
		/* attempt to split down the middle in terms of item+data size */
		split_at = node_split_point(node_addr);
		
		/* TODO: set new_node->hdr.leaf */
		
		/* copy items to the new node */
		for (uint16_t i = split_at; i < node->hdr.item_qty; ++i) {
			node_xfer(new_node, node, i);
		}
		
		/* zero out the transferred items and data */
		node_item_zero(node, split_at);
		
		first_key  = &node->items[0].key;
		second_key = &new_node->items[0].key;
	} else {
		uint16_t new_idx = 0;
		for (uint16_t i = split_at; i < node->hdr.item_qty; ++i) {
			new_node->children[new_idx] = node->children[i];
			
			++new_idx;
		}
		
		/* zero out the transferred node ptrs */
		uint8_t *zero_begin = (uint8_t *)&node->children[split_at];
		uint8_t *zero_end   = (uint8_t *)node + fs.blk_size;
		
		memset(zero_begin, 0, zero_end - zero_begin);
		
		first_key  = &node->children[0].key;
		second_key = &new_node->children[0].key;
	}
	
	new_node->hdr.item_qty = node->hdr.item_qty - split_at;
	node->hdr.item_qty = split_at;
	
	/* this is poorly written and needs to be redone */
	if (node->hdr.parent_addr == 0) {
		uint32_t newer_addr = ext_alloc(1);
		struct jgfs2_node *newer_node = fs_map_blk(newer_addr, 1);
		
		*newer_node = *node;
		newer_node->hdr.parent_addr = node_addr;
		
		/* zero out the new root node */
		uint8_t *zero_begin = (uint8_t *)&node->hdr +
			sizeof(struct jgfs2_node_hdr);
		uint8_t *zero_end   = (uint8_t *)node + fs.blk_size;
		
		memset(zero_begin, 0, zero_end - zero_begin);
		
		node->hdr.leaf = false;
		node->hdr.item_qty = 2;
		node->children[0].key = *first_key;
		node->children[0].addr = newer_addr;
		node->children[1].key = *second_key;
		node->children[1].addr = new_addr;
		
		fs_unmap_blk(newer_node, newer_addr, 1);
	} else {
		
	}
	
	/* create a new node with the second half of the data from this node */
		/* if this is not the root node (parent != 0), try to insert a ptr to
		 * the new node into our parent */
			/* if the parent is full, recurse */
		/* if this is the root node (parent == 0), clear it out and create two
		 * children, one for each half of the data */
	/* update all child nodes' parent value as necessary */
	
	TODO("update all hdr.next_addr");
	
	fs_unmap_blk(new_node, new_addr, 1);
	fs_unmap_blk(node, node_addr, 1);
}
#endif
