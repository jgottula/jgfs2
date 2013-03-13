/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../check.h"
#include "../../debug.h"


struct check_result check_leaf(leaf_ptr node) {
	struct check_result result = { RESULT_TYPE_OK };
	
	if (node->hdr.cnt > 0) {
		if (node->hdr.prev != 0) {
			node_ptr prev = node_map(node->hdr.prev, false);
			bool is_leaf = prev->hdr.leaf;
			node_unmap(prev);
			if (!is_leaf) {
				result.type = RESULT_TYPE_LEAF;
				result.leaf = (struct leaf_check_error){
					.code      = ERR_LEAF_PREV_BRANCH,
					.node_addr = node->hdr.this,
					
					.elem_cnt = 0,
				};
				
				goto done;
			}
		}
		if (node->hdr.next != 0) {
			node_ptr next = node_map(node->hdr.next, false);
			bool is_leaf = next->hdr.leaf;
			node_unmap(next);
			if (!is_leaf) {
				result.type = RESULT_TYPE_LEAF;
				result.leaf = (struct leaf_check_error){
					.code      = ERR_LEAF_NEXT_BRANCH,
					.node_addr = node->hdr.this,
					
					.elem_cnt = 0,
				};
				
				goto done;
			}
		}
		
		if (leaf_used(node) > node_size_usable()) {
			result.type = RESULT_TYPE_LEAF;
			result.leaf = (struct leaf_check_error){
				.code      = ERR_LEAF_OVERFLOW,
				.node_addr = node->hdr.this,
				
				.elem_cnt = 0,
			};
			
			goto done;
		}
		
		uint32_t last_off = node_size_byte();
		for (uint16_t i = 0; i < node->hdr.cnt; ++i) {
			const item_ref *elem = node->elems + i;
			
			if (last_off != elem->off + elem->len) {
				result.type = RESULT_TYPE_LEAF;
				result.leaf = (struct leaf_check_error){
					.code      = (last_off < elem->off + elem->len ?
						ERR_LEAF_UNCONTIG : ERR_LEAF_OVERLAP),
					.node_addr = node->hdr.this,
					
					.elem_cnt    = 1,
					.elem_idx[0] = i,
					.elem[0]     = *elem,
				};
				
				if (i < node->hdr.cnt - 1) {
					result.leaf.elem_cnt    = 2;
					result.leaf.elem_idx[1] = i + 1;
					result.leaf.elem[1]     = *(elem + 1);
				}
				
				goto done;
			}
			
			last_off -= elem->len;
		}
	}
	
	const item_ref *elem_end = node->elems + node->hdr.cnt;
	for (const item_ref *elem = node->elems; elem < elem_end; ++elem) {
		struct item_data item = {
			.len  = elem->len,
			.data = (uint8_t *)node + elem->off,
		};
		
		result = check_item(&elem->key, item);
		if (result.type != RESULT_TYPE_OK) {
			goto done;
		}
	}
	
done:
	return result;
}
