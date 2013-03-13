/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../check.h"
#include "../../debug.h"


struct check_result check_node(uint32_t node_addr, bool recurse) {
	struct check_result result = { RESULT_TYPE_OK };
	node_ptr node = node_map(node_addr, false);
	
	if (node->hdr.this != node_addr) {
		result.type = RESULT_TYPE_NODE;
		result.node = (struct node_check_error){
			.code      = ERR_NODE_THIS,
			.node_addr = node_addr,
			
			.elem_cnt = 0,
		};
		
		goto done;
	}
	
	/* if we are empty and not the root node, or if the root is an empty branch
	 * node, then this check fails */
	if (node->hdr.cnt == 0 && (node->hdr.parent != 0 || !node->hdr.leaf)) {
		result.type = RESULT_TYPE_NODE;
		result.node = (struct node_check_error){
			.code      = ERR_NODE_EMPTY,
			.node_addr = node_addr,
			
			.elem_cnt = 0,
		};
		
		goto done;
	}
	
	if (node->hdr.leaf) {
		leaf_ptr node_leaf = (leaf_ptr)node;
		
		const item_ref *elem_end = node_leaf->elems + node_leaf->hdr.cnt;
		for (const item_ref *elem = node_leaf->elems + 1;
			elem < elem_end; ++elem) {
			const item_ref *elem_prev = elem - 1;
			
			int8_t cmp = key_cmp(&elem_prev->key, &elem->key);
			
			if (cmp >= 0) {
				result.type = RESULT_TYPE_NODE;
				result.node = (struct node_check_error){
					.code      = (cmp > 0 ? ERR_NODE_SORT : ERR_NODE_DUPE),
					.node_addr = node_addr,
					
					.elem_cnt = 2,
					.elem_idx[0] = elem_prev - node_leaf->elems,
					.elem_idx[1] = elem - node_leaf->elems,
					.key[0]      = elem_prev->key,
					.key[1]      = elem->key,
				};
				
				goto done;
			}
		}
	} else {
		branch_ptr node_branch = (branch_ptr)node;
		
		const node_ref *elem_end = node_branch->elems + node_branch->hdr.cnt;
		for (const node_ref *elem = node_branch->elems + 1;
			elem < elem_end; ++elem) {
			const node_ref *elem_prev = elem - 1;
			
			int8_t cmp = key_cmp(&elem_prev->key, &elem->key);
			
			if (cmp >= 0) {
				result.type = RESULT_TYPE_NODE;
				result.node = (struct node_check_error){
					.code      = (cmp > 0 ? ERR_NODE_SORT : ERR_NODE_DUPE),
					.node_addr = node_addr,
					
					.elem_cnt = 2,
					.elem_idx[0] = elem_prev - node_branch->elems,
					.elem_idx[1] = elem - node_branch->elems,
					.key[0]      = elem_prev->key,
					.key[1]      = elem->key,
				};
				
				goto done;
			}
		}
	}
	
	/* run specific branch- and leaf-specific checks */
	if (node->hdr.leaf) {
		result = check_leaf((leaf_ptr)node);
	} else {
		result = check_branch((branch_ptr)node);
	}
	
done:
	node_unmap(node);
	
	return result;
}

#warning handle recurse in check_node
/*	if (recurse) {
		const node_ref *elem_end = node->elems + node->hdr.cnt;
		for (const node_ref *elem = node->elems; elem < elem_end; ++elem) {
			result = check_node(elem->addr, true);
			if (result.type != RESULT_TYPE_OK) {
				goto done;
			}
		}
	}*/
