/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../check.h"
#include "../../debug.h"


struct check_result check_leaf(const node_ptr leaf) {
	struct check_result result = { RESULT_TYPE_OK };
	
	if (leaf->hdr.cnt > 0) {
		if (leaf->hdr.prev != 0) {
			node_ptr prev = node_map(leaf->hdr.prev, false);
			bool is_leaf = prev->hdr.leaf;
			node_unmap(prev);
			if (!is_leaf) {
				result.type = RESULT_TYPE_LEAF;
				result.leaf = (struct leaf_check_error){
					.code      = ERR_LEAF_PREV_BRANCH,
					.leaf_addr = leaf->hdr.this,
					
					.elem_cnt = 0,
				};
				
				goto done;
			}
		}
		if (leaf->hdr.next != 0) {
			node_ptr next = node_map(leaf->hdr.next, false);
			bool is_leaf = next->hdr.leaf;
			node_unmap(next);
			if (!is_leaf) {
				result.type = RESULT_TYPE_LEAF;
				result.leaf = (struct leaf_check_error){
					.code      = ERR_LEAF_NEXT_BRANCH,
					.leaf_addr = leaf->hdr.this,
					
					.elem_cnt = 0,
				};
				
				goto done;
			}
		}
		
		uint32_t last_off = node_size_byte();
		for (uint16_t i = 0; i < leaf->hdr.cnt; ++i) {
			const item_ref *elem = leaf->l_elems + i;
			
			if (last_off != elem->off + elem->len) {
				result.type = RESULT_TYPE_LEAF;
				result.leaf = (struct leaf_check_error){
					.code      = (last_off < elem->off + elem->len ?
						ERR_LEAF_UNCONTIG : ERR_LEAF_OVERLAP),
					.leaf_addr = leaf->hdr.this,
					
					.elem_cnt    = 1,
					.elem_idx[0] = i,
					.elem[0]     = *elem,
				};
				
				if (i < leaf->hdr.cnt - 1) {
					result.leaf.elem_cnt    = 2;
					result.leaf.elem_idx[1] = i + 1;
					result.leaf.elem[1]     = *(elem + 1);
				}
				
				goto done;
			}
			
			last_off -= elem->len;
		}
	}
	
	const item_ref *elem_end = leaf->l_elems + leaf->hdr.cnt;
	for (const item_ref *elem = leaf->l_elems; elem < elem_end; ++elem) {
		struct item_data item = {
			.len  = elem->len,
			.data = leaf_elem_data(leaf, (elem - leaf->l_elems)),
		};
		
		result = check_item(&elem->key, item);
		if (result.type != RESULT_TYPE_OK) {
			goto done;
		}
	}
	
done:
	return result;
}
