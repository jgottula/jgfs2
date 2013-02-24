#include "tree.h"
#include <inttypes.h>
#include <stdio.h>
#include "debug.h"
#include "dev.h"
#include "extent.h"
#include "fs.h"


int8_t key_cmp(const struct jgfs2_key *lhs, const struct jgfs2_key *rhs) {
	if (lhs->id > rhs->id) {
		return 1;
	} else if (lhs->id < rhs->id) {
		return -1;
	}
	
	if (lhs->type > rhs->type) {
		return 1;
	} else if (lhs->type < rhs->type) {
		return -1;
	}
	
	if (lhs->off > rhs->off) {
		return 1;
	} else if (lhs->off < rhs->off) {
		return -1;
	}
	
	return 0;
}

bool node_item(uint32_t node_addr, const struct jgfs2_key *key,
	void **found_data) {
	struct jgfs2_node *node = fs_map_blk(node_addr, 1);
	bool status = false;
	
	if (!node->hdr.leaf) {
		errx(1, "%s: not a leaf node: %" PRIu32, __func__, node_addr);
	}
	
	for (uint16_t i = 0; i < node->hdr.item_qty; ++i) {
		const struct jgfs2_item *item = &node->items[i];
		
		if (key_cmp(key, &item->key) == 0) {
			*found_data = (uint8_t *)node + item->off;
		}
	}
	
	fs_unmap_blk(node, node_addr, 1);
	
	return status;
}

uint32_t node_space(uint32_t node_addr) {
	struct jgfs2_node *node = fs_map_blk(node_addr, 1);
	uint32_t space = 0;
	
	if (!node->hdr.leaf) {
		errx(1, "%s: not a leaf node: %" PRIu32, __func__, node_addr);
	}
	
	TODO("test this");
	
	intptr_t space_end =
		(intptr_t)node + node->items[node->hdr.item_qty - 1].off;
	intptr_t space_begin =
		(intptr_t)(&node->items[node->hdr.item_qty]);
	
	space = space_end - space_begin;
	
	fs_unmap_blk(node, node_addr, 1);
	
	return space;
}

uint32_t node_used(uint32_t node_addr) {
	struct jgfs2_node *node = fs_map_blk(node_addr, 1);
	uint32_t used = 0;
	
	if (!node->hdr.leaf) {
		errx(1, "%s: not a leaf node: %" PRIu32, __func__, node_addr);
	}
	
	TODO("test this");
	
	uint32_t used_items =
		node->hdr.item_qty * sizeof(struct jgfs2_item);
	uint32_t used_data_begin =
		node->items[node->hdr.item_qty - 1].off;
	
	used = used_items + (fs.blk_size - used_data_begin);
	
	fs_unmap_blk(node, node_addr, 1);
	
	return used;
}

void node_dump(uint32_t node_addr) {
	struct jgfs2_node *node = fs_map_blk(node_addr, 1);
	
	warnx("%s: %s node %" PRIu32 " with %" PRIu16 " %ss", __func__,
		(node->hdr.leaf ? "leaf" : "non-leaf"), node_addr, node->hdr.item_qty,
		(node->hdr.leaf ? "item" : "node ptr"));
	
	for (uint16_t i = 0; i < node->hdr.item_qty; ++i) {
		if (node->hdr.leaf) {
			const struct jgfs2_item *item = &node->items[i];
			
			fprintf(stderr, "item %3" PRIu16 ": id %" PRIu32 " type %" PRIu8 
				" off %" PRIu32 "\n",
				i, item->key.id, item->key.type, item->key.off);
		} else {
			const struct jgfs2_node_ptr *child = &node->children[i];
			
			fprintf(stderr, "child %3" PRIu16 ": id %" PRIu32 " type %" PRIu8 
				" off %" PRIu32 "\naddr %08" PRIx32,
				i, child->key.id, child->key.type, child->key.off, child->addr);
		}
	}
	
	uint8_t *node_bytes = (uint8_t *)node;
	for (size_t i = 0; i < fs.blk_size; ++i) {
		if (i % 16 == 0) {
			fprintf(stderr, "%04zx: ", i);
		} else if (i % 16 == 8) {
			fputc(' ', stderr);
		}
		
		fprintf(stderr, " %02x", node_bytes[i]);
		
		if (i % 16 == 15) {
			fputc('\n', stderr);
		}
	}
	
	fs_unmap_blk(node, node_addr, 1);
}

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
		uint32_t used = node_used(node_addr);
		uint32_t used_incr = 0;
		
		/* attempt to split down the middle in terms of item size */
		for (uint16_t i = 0; i < node->hdr.item_qty; ++i) {
			used_incr += sizeof(struct jgfs2_item);
			used_incr += node->items[i].len;
			
			if (i != 0 && used_incr >= used / 2) {
				split_at = i;
				break;
			}
		}
		
		/* copy items to the new node */
		uint16_t new_idx = 0;
		uint32_t data_off = fs.blk_size;
		for (uint16_t i = split_at; i < node->hdr.item_qty; ++i) {
			const struct jgfs2_item *item = &node->items[i];
			
			data_off -= node->items[i].len;
			memcpy((uint8_t *)new_node + data_off, (uint8_t *)node + item->off,
				item->len);
			
			new_node->items[new_idx] = node->items[i];
			new_node->items[new_idx].off = data_off;
			
			++new_idx;
		}
		
		/* zero out the transferred items and data */
		uint8_t *zero_begin = (uint8_t *)&node->items[split_at];
		uint8_t *zero_end   = (uint8_t *)node + node->items[split_at].off +
			node->items[split_at].len;
		
		memset(zero_begin, 0, zero_end - zero_begin);
		
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
