/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../check.h"
#include "../../debug.h"


struct check_result check_branch(branch_ptr node) {
	struct check_result result = { RESULT_TYPE_OK };
	
	if (branch_used(node) > node_size_usable()) {
		result.type = RESULT_TYPE_BRANCH;
		result.branch = (struct branch_check_error){
			.code      = ERR_BRANCH_OVERFLOW,
			.node_addr = node->hdr.this,
			
			.elem_cnt = 0,
		};
		
		goto done;
	}
	
	for (uint16_t i = 0; i < node->hdr.cnt; ++i) {
		const node_ref *elem = node->elems + i;
		node_ptr child = node_map(elem->addr, false);
		
		bool bad = false;
		uint32_t code = 0;
		if (child->hdr.parent != node->hdr.this) {
			bad = true;
			code = ERR_BRANCH_PARENT;
		} else if (child->hdr.cnt == 0) {
			bad = true;
			code = ERR_BRANCH_EMPTY_CHILD;
		} else if (key_cmp(&elem->key, node_first_key(child)) != 0) {
			bad = true;
			code = ERR_BRANCH_KEY;
		}
		
		node_unmap(child);
		
		if (bad) {
			result.type = RESULT_TYPE_BRANCH;
			result.branch = (struct branch_check_error){
				.code      = code,
				.node_addr = node->hdr.this,
				
				.elem_cnt = 1,
				.elem_idx = i,
				.elem     = *elem,
			};
			
			goto done;
		}
	}
	
done:
	return result;
}
