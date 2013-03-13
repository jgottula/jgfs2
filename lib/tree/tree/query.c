/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../tree.h"
#include "../../debug.h"


static leaf_ptr tree_search_r(uint32_t root_addr, uint32_t node_addr,
	const key *key) {
	node_ptr node = node_map(node_addr, true);
	
	/* remove this later for performance */
	#warning
	//check_node(node_addr, false);
	
	if (node->hdr.leaf) {
		return (leaf_ptr)node;
	} else {
		branch_ptr branch = (branch_ptr)node;
		
		uint32_t child_addr = branch_search(branch, key);
		leaf_ptr child = tree_search_r(root_addr, child_addr, key);
		
		node_unmap(node);
		return child;
	}
}

leaf_ptr tree_search(uint32_t root_addr, const key *key) {
	ASSERT_ROOT(root_addr);
	
	//tree_lock(root_addr);
	leaf_ptr result = tree_search_r(root_addr, root_addr, key);
	//tree_unlock(root_addr);
	
	return result;
}

bool tree_retrieve(uint32_t root_addr, const key *key, size_t max_len,
	void *buf) {
	ASSERT_ROOT(root_addr);
	tree_lock(root_addr);
	
	bool result = false;
	leaf_ptr leaf = tree_search_r(root_addr, root_addr, key);
	uint16_t idx;
	if (node_search((node_ptr)leaf, key, &idx)) {
		item_ref *elem = leaf->elems + idx;
		
		if (elem->len <= max_len) {
			uint8_t *data_ptr = leaf_data_ptr(leaf, idx);
			memcpy(buf, data_ptr, elem->len);
			
			result = true;
		}
	}
	
	node_unmap((node_ptr)leaf);
	tree_unlock(root_addr);
	
	return result;
}
