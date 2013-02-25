#include "check.h"
#include "../debug.h"


struct check_result check_item(const key *key, struct item_data item) {
	struct check_result result = { RESULT_TYPE_OK };
	
	// test here
	// when any test fails, set result and goto done
	
//done:
	return result;
}

struct check_result check_node_branch(branch_ptr node) {
	struct check_result result = { RESULT_TYPE_OK };
	
	// test here
	// when any test fails, set result and goto done
	
//done:
	return result;
}

struct check_result check_node_leaf(leaf_ptr node) {
	struct check_result result = { RESULT_TYPE_OK };
	
	// test here
	// when any test fails, set result and goto done
	
//done:
	return result;
}

struct check_result check_node(uint32_t node_addr) {
	struct check_result result = { RESULT_TYPE_OK };
	node_ptr node = node_map(node_addr);
	
	if (node->hdr.this != node_addr) {
		result.type = RESULT_TYPE_NODE;
		result.node = (struct node_check_error){
			.code        = ERR_NODE_THIS,
			.node_addr   = node_addr,
		};
		
		goto done;
	}
	
	if (node->hdr.parent != 0 && node->hdr.cnt == 0) {
		result.type = RESULT_TYPE_NODE;
		result.node = (struct node_check_error){
			.code        = ERR_NODE_EMPTY,
			.node_addr   = node_addr,
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
		branch_ptr node_branch = (branch_ptr)node;
		
		const node_ref *elem_end = node_branch->elems + node_branch->hdr.cnt;
		for (const node_ref *elem = node_branch->elems + 1;
			elem < elem_end; ++elem) {
			const node_ref *elem_prev = elem - 1;
			
			int8_t cmp = key_cmp(&elem_prev->key, &elem->key);
			
			if (cmp >= 0) {
				result.type = RESULT_TYPE_NODE;
				result.node = (struct node_check_error){
					.code        = (cmp > 0 ? ERR_NODE_SORT : ERR_NODE_DUPE),
					.node_addr   = node_addr,
					
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
		result = check_node_branch((branch_ptr)node);
	} else {
		result = check_node_leaf((leaf_ptr)node);
	}
	
done:
	node_unmap(node);
	
	return result;
}

struct check_result check_tree(uint32_t root_addr) {
	struct check_result result = { RESULT_TYPE_OK };
	
	// run node_check on each node in the tree
	
	// check for overall tree consistency:
	// dupes between nodes
	// node n's largest key not less than node n+1's smallest
	
//done:
	return result;
}

void check_print(struct check_result result, bool fatal) {
	if (result.type == RESULT_TYPE_OK) {
		return;
	} else if (result.type == RESULT_TYPE_TREE) {
		//const struct tree_check_error *err = &result.tree;
		
		TODO("report for tree results");
	} else if (result.type == RESULT_TYPE_NODE) {
		const struct node_check_error *err = &result.node;
		const node_ptr node = node_map(err->node_addr);
		
		warnx("check_node on 0x%" PRIx32 " failed:", err->node_addr);
		
		bool have_desc = true;
		const char *err_desc;
		switch (err->code) {
		case ERR_NODE_THIS:
			err_desc = "hdr.this != node_addr";
			break;
		case ERR_NODE_EMPTY:
			err_desc = "empty non-root node";
			break;
		case ERR_NODE_SORT:
			err_desc = "key sort violation";
			break;
		case ERR_NODE_DUPE:
			err_desc = "key dupe";
			break;
		default:
			have_desc = false;
		}
		
		if (have_desc) {
			warnx("%s", err_desc);
		} else {
			warnx("unknown error (%" PRId32 ")", err->code);
			err_desc = "unknown error";
		}
		
		switch (err->code) {
		case ERR_NODE_THIS:
			warnx("hdr.this 0x%" PRIx32, node->hdr.this);
			break;
		case ERR_NODE_SORT:
		case ERR_NODE_DUPE:
			warnx("[elem %" PRIu16 "] key %s",
				err->elem_idx[0], key_str(&err->key[0]));
			warnx("[elem %" PRIu16 "] key %s",
				err->elem_idx[1], key_str(&err->key[1]));
			break;
		}
	} else if (result.type == RESULT_TYPE_BRANCH) {
		//const struct branch_check_error *err = &result.branch;
		
		TODO("report for branch results");
	} else if (result.type == RESULT_TYPE_LEAF) {
		//const struct leaf_check_error *err = &result.leaf;
		
		TODO("report for leaf results");
	} else if (result.type == RESULT_TYPE_ITEM) {
		//const struct item_check_error *err = &result.item;
		
		TODO("report for item results");
	} else {
		errx("%s: result.type unknown: %" PRIu32, __func__, result.type);
	}
	
	if (fatal) {
		errx("cannot continue");
	}
}
