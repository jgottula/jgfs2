#include "check.h"


struct check_result check_item(const key *key, struct item_data item) {
	
}

struct check_result check_node_branch(branch_ptr node) {
	struct check_result result = { RESULT_TYPE_OK };
	
	// test here
	// when any test fails, set result and goto done
	
	// ERR_BRANCH_KEY_MISMATCH means that a subbranch's key isn't equal to its
	// first key, as it ought to be
	
done:
	return result;
}

struct check_result check_node_leaf(leaf_ptr node) {
	struct check_result result = { RESULT_TYPE_OK };
	
	// test here
	// when any test fails, set result and goto done
	
	// ERR_BRANCH_KEY_MISMATCH means that a subbranch's key isn't equal to its
	// first key, as it ought to be
	
done:
	return result;
}

struct check_result check_node(uint32_t node_addr) {
	struct check_result result = { RESULT_TYPE_OK };
	node_ptr node = node_map(node_addr);
	
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
					.code        = (cmp > 0 ? ERR_NODE_SORT : ERR_NODE_DUPE),
					.node_addr   = node_addr,
					.elem_idx[0] = elem_prev - node_leaf->elems,
					.elem_idx[1] = elem - node_leaf->elems,
					.key[0]      = elem_prev->key,
					.key[1]      = elem->key,
				};
				
				goto done;
			}
		}
	} else {
		
	}
	
	/* run specific branch- and leaf-specific checks */
	if (node->hdr.leaf) {
		result = check_node_branch((branch_ptr)node);
	} else {
		result = check_node_leaf((leaf_ptr)node);
	}
	
done:
	node_unmap(node);
	
	return result;
}

struct check_result check_tree(uint32_t root_addr) {
	// run node_check on each node in the tree
	
	// check for overall tree consistency:
	// dupes between nodes
	// node n's largest key not less than node n+1's smallest
}
