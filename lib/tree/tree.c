#include "tree.h"
#include "../debug.h"
#include "../dev.h"
#include "../extent.h"
#include "../fs.h"


void tree_init(uint32_t root_addr) {
	struct jgfs2_node *root = fs_map_blk(root_addr, 1);
	
	root->hdr.leaf        = true;
	root->hdr.parent_addr = 0;
	root->hdr.next_addr   = 0;
	root->hdr.item_qty    = 0;
	
	fs_unmap_blk(root, root_addr, 1);
}

void tree_split(uint32_t node_addr) {
	struct jgfs2_node *node = fs_map_blk(node_addr, 1);
	
	TODO("test this");
	
	
	
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

void tree_insert(uint32_t node_addr, const struct jgfs2_key *key, uint32_t len,
	void *data) {
	uint32_t dest_addr = tree_search(node_addr, key);
	
	if (node_space(dest_addr) >= sizeof(struct jgfs2_item) + len) {
		/* okay to insert normally */
	} else {
		/* need to split the node */
	}
}

uint32_t tree_search(uint32_t node_addr, const struct jgfs2_key *key) {
	struct jgfs2_node *node = fs_map_blk(node_addr, 1);
	uint32_t found_addr = 0;
	
	if (node->hdr.leaf) {
		found_addr = node_addr;
		goto done;
	} else {
		/* TODO: handle case where key is less than the lowest subbranch key */
		
		for (uint16_t i = 0; i < node->hdr.item_qty; ++i) {
			if (key_cmp(key, &node->children[i].key) >= 0) {
				if (i == node->hdr.item_qty - 1 ||
					key_cmp(key, &node->children[i + 1].key) < 0) {
					found_addr = tree_search(node->children[i].addr, key);
					goto done;
				}
			}
		}
	}
	
done:
	fs_unmap_blk(node, node_addr, 1);
	
	return found_addr;
}
