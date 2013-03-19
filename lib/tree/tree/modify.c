/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../tree.h"
#include "../../debug.h"


static bool tree_insert_sibling(node_ptr node, uint32_t space_needed,
	const key *key, union elem_payload payload) {
	bool result = false;
	
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
	
	if (free_prev + free_next < space_needed) {
		goto done;
	}
	
	/* were we able to export to a sibling last time? */
	bool prev_avail = prev_exist;
	bool next_avail = next_exist;
	
	/* space made available by exporting to siblings */
	uint32_t space_total = 0;
	uint32_t space_prev  = 0;
	uint32_t space_next  = 0;
	
	/* number of elems exported to siblings */
	uint16_t cnt_prev = 0;
	uint16_t cnt_next = 0;
	
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
					uint32_t size_this = node_elem_weight(node, idx_prev);
					
					if (size_this <= free_prev) {
						free_prev   -= size_this;
						space_total += size_this;
						space_prev  += size_this;
						
						++cnt_prev;
						
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
					uint32_t size_this = node_elem_weight(node, idx_next);
					
					if (size_this <= free_next) {
						free_next   -= size_this;
						space_total += size_this;
						space_next  += size_this;
						
						++cnt_next;
						
						--idx_next;
					} else {
						next_avail = false;
					}
				} else {
					next_avail = false;
				}
			}
			
			/* try the other sibling after this one */
			try_prev = !try_prev;
			try_next = !try_next;
		}
	} while (space_total < space_needed && (prev_avail || next_avail));
	
	
	// NOTE: the code from here on out needs some help.
	
	if (space_total < space_needed) {
		goto done;
	}
	
	
	// NOTE: we are using 'space' for 'diff_data' and this is wrong: 'space'
	// includes bytes used for the item_ref itself
	
	
	/* number of elems remaining to the sides of the insertion point */
	uint16_t cnt_rem_left  = (idx_insert - idx_prev);
	uint16_t cnt_rem_right = (idx_next - idx_insert) + 1;
	
	if (cnt_prev > 0) {
		/* append to prev node */
		node_append_multiple(prev, node, 0, cnt_prev, space_prev);
		
		/* shift remaining elems before the insertion point */
		if (cnt_rem_left != 0) {
			node_shift_backward(node, idx_prev, idx_insert - 1, cnt_prev,
				space_prev);
		}
		
		// TODO: update this node's parent ref
	}
	if (cnt_next > 0) {
		/* prepend to next node */
		node_prepend_multiple(next, node, idx_next + 1, cnt_next,
			space_next);
		
		// WHAT IF: cnt_prev == 0 and we would insert into index 0??
		
		/* shift remaining elems after the insertion point, leaving space
		 * for the item to be inserted */
		if (cnt_rem_right != 0 && cnt_prev > 1) {
			node_shift_backward(node, idx_insert, idx_next, cnt_prev - 1,
				space_next);
		}
		
		
		// TODO: update next node's parent ref
	}
	
	
	// zero?
	
	// put key in
	
	// update this cnt
	
	// fill in data
	
	result = true;
	
done:
	/* unmap sibling node pointers */
	prev_exist ? node_unmap(prev) : (void)0;
	next_exist ? node_unmap(next) : (void)0;
	
	return result;
}

static bool tree_insert_normal(node_ptr node, uint32_t space_needed,
	const key *key, union elem_payload payload) {
	if (node_free(node) < space_needed) {
		return false;
	}
	
	uint16_t idx_insert = node_search_hypo(node, key);
	if (idx_insert < node->hdr.cnt) {
		uint32_t diff_data = (node->hdr.leaf ? payload.l_item.len : 0);
		node_shift_forward(node, idx_insert, node->hdr.cnt - 1, 1, diff_data);
	}
	
	++node->hdr.cnt;
	node_elem_fill(node, idx_insert, key, payload);
	
	if (idx_insert == 0) {
		node_update_ref_in_parent(node);
	}
	
	return true;
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
	
	/* try normal and sibling-export inserts, then split if both fail */
	if (!tree_insert_normal(node, space_needed, key, payload) &&
		!tree_insert_sibling(node, space_needed, key, payload)) {
		errx("not done yet");
		// use node_ptr * so that the split functions can reassign the ptr
		
		// if have neighbors, pick the one with more junk (or next if equal)
		//  and do a 2->3 split
		// if no neighbors, do a normal btree split (rightward)
		
		// split will return ptr to node with key
		// if leaf, need to set elem data
	}
	
	node_unmap(node);
}

void tree_insert(uint32_t root_addr, const key *key, struct item_data item) {
	ASSERT_ROOT(root_addr);
	tree_lock(root_addr);
	
	if (item.len * 10 >= node_size_usable()) {
		warnx("%s: item len >= 10%% of node size: root 0x%" PRIx32
			" key %s len %" PRIu32,
			__func__, root_addr, key_str(key), item.len);
	}
	
	node_ptr leaf = tree_search(root_addr, key);
	uint32_t leaf_addr = leaf->hdr.this;
	tree_insert_r(root_addr, leaf_addr, key, (union elem_payload){
		.l_item = item,
	});
	node_unmap(leaf);
	
	tree_unlock(root_addr);
}

void tree_remove(uint32_t root_addr, const key *key) {
	ASSERT_ROOT(root_addr);
	tree_lock(root_addr);
	
	
	
	// condition for merging nodes: "if it's possible to do, then do it"
	// it may be hard to determine if the node sweep is compressible
	//  ALL sweeps that are dense (have too many bytes to possibly fit into n-1
	//   nodes) can immediately be disregarded as incompressible
	//  sweeps that are not dense, but are compressible, will take a little work
	
	// even easier: for branch merges, the compressibility criterion is trivial
	
	// do 3->2 merge if possible
	
	
	tree_unlock(root_addr);
}
