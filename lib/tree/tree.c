#include "tree.h"
#include "../debug.h"
#include "check.h"


void tree_init(uint32_t root_addr) {
	node_unmap((node_ptr)leaf_init(root_addr, 0, 0));
}

void tree_insert(uint32_t root_addr, const key *key, struct item_data item) {
	bool done = false;
	do {
		leaf_ptr leaf      = tree_search(root_addr, key);
		uint32_t leaf_addr = leaf->hdr.this;
		
		if (leaf_insert(leaf, key, item)) {
			done = true;
		} else {
			node_unmap((node_ptr)leaf);
			node_split(leaf_addr);
		}
	} while (!done);
}

static leaf_ptr tree_search_r(uint32_t root_addr, uint32_t node_addr,
	const key *key) {
	node_ptr node = node_map(node_addr);
	
	/* remove this later for performance */
	check_node(node_addr);
	
	if (node->hdr.leaf) {
		return (leaf_ptr)node;
	} else {
		branch_ptr branch = (branch_ptr)node;
		leaf_ptr   result = NULL;
		
		if (node->hdr.cnt == 0) {
			errx(1, "%s: cannot be empty: root %08" PRIx32 " node %08" PRIx32,
				__func__, root_addr, node_addr);
		}
		
		/* if smaller than any other key, recurse through the first subnode */
		const node_ref *elem_first = branch->elems;
		if (key_cmp(key, &elem_first->key) < 0) {
			result = tree_search_r(root_addr, elem_first->addr, key);
		} else {
			const node_ref *elem_end = branch->elems + branch->hdr.cnt;
			for (const node_ref *elem = branch->elems;
				elem < elem_end; ++elem) {
				if (key_cmp(key, &elem->key) > 0) {
					result = tree_search_r(root_addr, elem->addr, key);
					break;
				}
			}
		}
		
		node_unmap(node);
		
		return result;
	}
}

leaf_ptr tree_search(uint32_t root_addr, const key *key) {
	node_ptr node = node_map(root_addr);
	if (node->hdr.parent != 0) {
		errx(1, "%s: node 0x%08" PRIx32 " not root: parent 0x%08" PRIx32,
			__func__, root_addr, node->hdr.parent);
	}
	node_unmap(node);
	
	return tree_search_r(root_addr, root_addr, key);
}

uint32_t tree_find_root(uint32_t node_addr) {
	uint32_t parent_addr = node_addr;
	
	do {
		node_addr = parent_addr;
		
		node_ptr node = node_map(node_addr);
		parent_addr = node->hdr.parent;
		node_unmap(node);
	} while (parent_addr != 0);
	
	return node_addr;
}