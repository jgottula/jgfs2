#include "tree.h"
#include <inttypes.h>
#include <stdio.h>
#include "debug.h"
#include "dev.h"
#include "fs.h"


int8_t jgfs2_key_cmp(const struct jgfs2_key *lhs, const struct jgfs2_key *rhs) {
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

bool jgfs2_node_item(uint32_t node_addr, const struct jgfs2_key *key,
	void **found_data) {
	struct jgfs2_node *node = jgfs2_fs_map_blk(node_addr, 1);
	bool status = false;
	
	if (!node->hdr.leaf) {
		errx(1, "%s: not a leaf node: %" PRIu32, __func__, node_addr);
	}
	
	for (uint16_t i = 0; i < node->hdr.item_qty; ++i) {
		const struct jgfs2_item *item = &node->items[i];
		
		if (jgfs2_key_cmp(key, &item->key) == 0) {
			*found_data = (uint8_t *)node + item->off;
		}
	}
	
	jgfs2_fs_unmap_blk(node, node_addr, 1);
	
	return status;
}

uint32_t jgfs2_node_space(uint32_t node_addr) {
	struct jgfs2_node *node = jgfs2_fs_map_blk(node_addr, 1);
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
	
	jgfs2_fs_unmap_blk(node, node_addr, 1);
	
	return space;
}

void jgfs2_node_dump(uint32_t node_addr) {
	struct jgfs2_node *node = jgfs2_fs_map_blk(node_addr, 1);
	
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
	
	jgfs2_fs_unmap_blk(node, node_addr, 1);
}

void jgfs2_tree_init(uint32_t root_addr) {
	struct jgfs2_node *root = jgfs2_fs_map_blk(root_addr, 1);
	
	root->hdr.leaf        = true;
	root->hdr.parent_addr = 0;
	root->hdr.next_addr   = 0;
	root->hdr.item_qty    = 0;
	
	jgfs2_fs_unmap_blk(root, root_addr, 1);
}

void jgfs2_tree_split(uint32_t node_addr) {
	/* if this is the root node (parent == 0), create a new root with two node
	 * ptrs: one to this node, and one to a new node */
	/* go to this node's parent and try to insert a key */
	/* if the parent is full, recurse */
	
	/* BE SURE TO UPDATE VALUES in node->hdr */
}

void jgfs2_tree_insert(uint32_t node_addr, const struct jgfs2_key *key,
	uint32_t len, void *data) {
	uint32_t dest_addr = jgfs2_tree_search(node_addr, key);
	
	if (jgfs2_node_space(dest_addr) >= sizeof(struct jgfs2_item) + len) {
		/* okay to insert normally */
	} else {
		/* need to split the node */
	}
}

uint32_t jgfs2_tree_search(uint32_t node_addr, const struct jgfs2_key *key) {
	struct jgfs2_node *node = jgfs2_fs_map_blk(node_addr, 1);
	uint32_t found_addr = 0;
	
	if (node->hdr.leaf) {
		found_addr = node_addr;
		goto done;
	} else {
		for (uint16_t i = 0; i < node->hdr.item_qty; ++i) {
			if (jgfs2_key_cmp(key, &node->children[i].key) >= 0) {
				if (i == node->hdr.item_qty - 1 ||
					jgfs2_key_cmp(key, &node->children[i + 1].key) < 0) {
					found_addr = jgfs2_tree_search(node->children[i].addr,key);
					goto done;
				}
			}
		}
	}
	
done:
	jgfs2_fs_unmap_blk(node, node_addr, 1);
	
	return found_addr;
}
