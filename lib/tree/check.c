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
	node_ptr node = node_map(node_addr);
	struct check_result result = { RESULT_TYPE_OK };
	
	// test here
	// when any test fails, set result and goto done
	
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
