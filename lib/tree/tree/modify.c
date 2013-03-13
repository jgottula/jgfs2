/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../tree.h"
#include "../../debug.h"


void tree_insert(uint32_t root_addr, const key *key, struct item_data item) {
	ASSERT_ROOT(root_addr);
	
	bool done = false;
	uint8_t retry = 0;
	do {
		leaf_ptr leaf = tree_search(root_addr, key);
		uint32_t leaf_addr = leaf->hdr.this;
		
		tree_lock(root_addr);
		
		if (leaf_insert(leaf, key, item)) {
			node_unmap((node_ptr)leaf);
			
			done = true;
		} else if (retry == 0) {
			node_unmap((node_ptr)leaf);
			node_split(leaf_addr);
			
			++retry;
		} else {
			errx("%s: giving up: root 0x%" PRIx32 " leaf 0x%"
				PRIx32 " %s len %" PRIu32,
				__func__, root_addr, leaf_addr, key_str(key), item.len);
		}
		
		tree_unlock(root_addr);
	} while (!done);
}

bool tree_remove(uint32_t root_addr, const key *key) {
	ASSERT_ROOT(root_addr);
	tree_lock(root_addr);
	
	leaf_ptr leaf = tree_search(root_addr, key);
	uint32_t leaf_addr = leaf->hdr.this;
	
	bool result = leaf_remove(leaf, key);
	node_unmap((node_ptr)leaf);
	
	tree_unlock(root_addr);
	
	return result;
}
