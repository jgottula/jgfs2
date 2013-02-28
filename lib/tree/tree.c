/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "tree.h"
#include "../debug.h"
#include "check.h"


#define ASSERT_ROOT(_node_addr) \
	if (!node_is_root(_node_addr)) { \
		errx("%s: not root: node 0x%" PRIx32, __func__, _node_addr); \
	}


void tree_dump(uint32_t root_addr) {
	ASSERT_ROOT(root_addr);
	TODO("implement this");
}

void tree_init(uint32_t root_addr) {
	node_unmap((node_ptr)leaf_init(root_addr, 0, 0, 0));
}

void tree_insert(uint32_t root_addr, const key *key, struct item_data item) {
	ASSERT_ROOT(root_addr);
	
	bool done = false, retried = false;
	do {
		leaf_ptr leaf      = tree_search(root_addr, key);
		uint32_t leaf_addr = leaf->hdr.this;
		
		if (leaf_insert(leaf, key, item)) {
			node_unmap((node_ptr)leaf);
			
			done = true;
		} else if (!retried) {
			node_unmap((node_ptr)leaf);
			node_split(leaf_addr);
			
			retried = true;
		} else {
			errx("%s: leaf_insert split ineffective: root 0x%" PRIx32
				" leaf 0x%" PRIx32 " %s len %" PRId32,
				__func__, root_addr, leaf_addr, key_str(key), item.len);
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
			errx("%s: cannot be empty: root 0x%" PRIx32 " node 0x%" PRIx32,
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
	ASSERT_ROOT(root_addr);
	return tree_search_r(root_addr, root_addr, key);
}
