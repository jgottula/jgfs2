/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../tree.h"
#include "../../debug.h"


static bool tree_make_space(node_ptr node, uint32_t space_needed,
	const key *key, union elem_payload payload) {
	/* do the siblings even exist? */
	bool prev_exist = (node->hdr.prev != 0);
	bool next_exist = (node->hdr.next != 0);
	if (!prev_exist && !next_exist) {
		return false;
	}
	
	/* sibling node pointers */
	node_ptr prev = (prev_exist ? node_map(node->hdr.prev, true) : NULL);
	node_ptr next = (next_exist ? node_map(node->hdr.next, true) : NULL);
	
	/* room available in siblings */
	uint32_t free_prev = (prev_exist ? node_free(prev) : 0);
	uint32_t free_next = (next_exist ? node_free(next) : 0);
	
	/* were we able to export to a sibling last time? */
	bool prev_avail = prev_exist;
	bool next_avail = next_exist;
	
	/* space made available by exporting to siblings */
	uint32_t space_total = 0;
	uint32_t space_prev  = 0;
	uint32_t space_next  = 0;
	
	/* indexes move inward toward the insertion point */
	uint16_t idx_insert = node_search_hypo(node, key);
	uint16_t idx_prev   = 0;
	uint16_t idx_next   = node->hdr.cnt - 1;
	do {
		/* try whichever sibling has more space first; prefer prev if equal */
		bool try_prev = (free_prev >= free_next ? true : false);
		bool try_next = !try_prev;
		for (uint8_t try = 2; try > 0; --try) {
			if (try_prev && prev_avail) {
				if (idx_prev < idx_insert) {
					uint32_t size_this = elem_weight(node, idx_prev);
					
					if (size_this <= free_prev) {
						free_prev   -= size_this;
						space_total += size_this;
						space_prev  += size_this;
						
						++idx_prev;
					} else {
						prev_avail = false;
					}
				} else {
					prev_avail = false;
				}
			}
			
			if (next_avail) {
				if (idx_next >= idx_insert) {
					uint32_t size_this = elem_weight(node, idx_next);
					
					if (size_this <= free_next) {
						free_next   -= size_this;
						space_total += size_this;
						space_next  += size_this;
						
						--idx_next;
					} else {
						next_avail = false;
					}
				} else {
					next_avail = false;
				}
			}
			
			try_prev = !try_prev;
			try_next = !try_next;
		}
	} while (space_total < space_needed && (prev_avail || next_avail));
	
	bool result = false;
	if (space_total >= space_needed) {
		result = true;
		
		// make space
		// do the insertion
	}
	
	/* update parent references */
	TODO("parent refs");
	
	/* unmap sibling node pointers */
	(prev_exist ? node_unmap(prev) : (void)0);
	(next_exist ? node_unmap(next) : (void)0);
	
	return result;
}

static void tree_insert_r(uint32_t root_addr, uint32_t node_addr,
	const key *key, union elem_payload payload) {
	node_ptr node = node_map(node_addr, true);
	
	uint32_t space_needed;
	if (node->hdr.leaf) {
		space_needed = sizeof(item_ref) + payload.l_item.len;
	} else {
		space_needed = sizeof(node_ref);
	}
	
	if (node_free(node) >= space_needed) {
		// node_insert
	} else if (!tree_make_space(node, space_needed, key, payload)) {
		// if have neighbors, pick the one with more junk (or next if equal)
		//  and do a 2->3 split
		// if no neighbors, do a normal btree split (rightward)
		
		// be sure to update parent refs at the end of the split
	}
	
	node_unmap(node);
}

void tree_insert(uint32_t root_addr, const key *key, struct item_data item) {
	ASSERT_ROOT(root_addr);
	
	node_ptr leaf = tree_search(root_addr, key);
	uint32_t leaf_addr = leaf->hdr.this;
	tree_insert_r(root_addr, leaf_addr, key, (union elem_payload){
		.l_item = item,
	});
	node_unmap(leaf);
	
#if 0
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
#endif
}

bool tree_remove(uint32_t root_addr, const key *key) {
	ASSERT_ROOT(root_addr);
	tree_lock(root_addr);
	
	/*leaf_ptr leaf = tree_search(root_addr, key);
	uint32_t leaf_addr = leaf->hdr.this;
	
	*/bool result/* = leaf_remove(leaf, key);
	node_unmap((node_ptr)leaf)*/;
	
	
	
	// condition for merging nodes: "if it's possible to do, then do it"
	// it may be hard to determine if the node sweep is compressible
	//  ALL sweeps that are dense (have too many bytes to possibly fit into n-1
	//   nodes) can immediately be disregarded as incompressible
	//  sweeps that are not dense, but are compressible, will take a little work
	
	// even easier: for branch merges, the compressibility criterion is trivial
	
	// do 3->2 merge if possible
	
	
	tree_unlock(root_addr);
	return result;
}
