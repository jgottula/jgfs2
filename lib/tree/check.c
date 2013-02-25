#include "check.h"
#include "../debug.h"


struct check_result check_tree(uint32_t root_addr) {
	struct check_result result = { RESULT_TYPE_OK };
	
	// run node_check on each node in the tree
	
	// check for overall tree consistency:
	// dupes between nodes
	// node n's largest key not less than node n+1's smallest
	
//done:
	return result;
}

struct check_result check_node(uint32_t node_addr) {
	struct check_result result = { RESULT_TYPE_OK };
	node_ptr node = node_map(node_addr);
	
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
		result = check_node_leaf((leaf_ptr)node);
	} else {
		result = check_node_branch((branch_ptr)node);
	}
	
done:
	node_unmap(node);
	
	return result;
}

struct check_result check_node_branch(branch_ptr node) {
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
		node_ptr child = node_map(elem->addr);
		
		bool bad = false;
		uint32_t code = 0;
		if (child->hdr.parent != node->hdr.this) {
			bad = true;
			code = ERR_BRANCH_PARENT;
		} else if (child->hdr.cnt == 0) {
			bad = true;
			code = ERR_BRANCH_EMPTY_CHILD;
		} else if ((child->hdr.leaf && key_cmp(&elem->key,
			&((leaf_ptr)child)->elems[0].key) != 0) ||
			(!child->hdr.leaf && key_cmp(&elem->key,
			&((branch_ptr)child)->elems[0].key) != 0)) {
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

struct check_result check_node_leaf(leaf_ptr node) {
	struct check_result result = { RESULT_TYPE_OK };
	
	if (node->hdr.cnt > 0) {
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

struct check_result check_item(const key *key, struct item_data item) {
	struct check_result result = { RESULT_TYPE_OK };
	
	// test here
	// when any test fails, set result and goto done
	
	// check: type validity
	// check: key.id appropriateness
	// check: key.off appropriateness
	// check: size appropriateness
	// check: contents of items by type
	
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
		}
		
		for (uint16_t i = 0; i < err->elem_cnt; ++i) {
			warnx("[elem %" PRIu16 "] key %s",
				err->elem_idx[1], key_str(&err->key[1]));
		}
		
		node_unmap(node);
	} else if (result.type == RESULT_TYPE_BRANCH) {
		const struct branch_check_error *err = &result.branch;
		const branch_ptr node = (branch_ptr)node_map(err->node_addr);
		
		warnx("check_branch on 0x%" PRIx32 " failed:", err->node_addr);
		
		bool have_desc = true;
		const char *err_desc;
		switch (err->code) {
		case ERR_BRANCH_OVERFLOW:
			err_desc = "node content overflow";
			break;
		case ERR_BRANCH_PARENT:
			err_desc = "child confused about paternity";
			break;
		case ERR_BRANCH_EMPTY_CHILD:
			err_desc = "empty child";
			break;
		case ERR_BRANCH_KEY:
			err_desc = "wrong key in node_ref";
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
		
		node_ptr child;
		if (err->elem_cnt > 0) {
			child = node_map(err->elem.addr);
			
			warnx("[elem %" PRIu16 "] key %s addr 0x%" PRIx32,
				err->elem_idx, key_str(&err->elem.key), err->elem.addr);
		}
		
		switch (err->code) {
		case ERR_BRANCH_OVERFLOW:
			warnx("%" PRIu32 " used > %" PRIu32 " possible",
				branch_used(node), node_size_usable());
			break;
		case ERR_BRANCH_PARENT:
			warnx("child believes its parent is 0x%" PRIx32, child->hdr.parent);
			break;
		case ERR_BRANCH_KEY:
		{
			const char *key;
			if (child->hdr.leaf) {
				key = key_str(&((leaf_ptr)child)->elems[0].key);
			} else {
				key = key_str(&((branch_ptr)child)->elems[0].key);
			}
			warnx("actual first key: %s", key);
			break;
		}
		}
		
		if (err->elem_cnt > 0) {
			node_unmap(child);
		}
		node_unmap((node_ptr)node);
	} else if (result.type == RESULT_TYPE_LEAF) {
		const struct leaf_check_error *err = &result.leaf;
		const leaf_ptr node = (leaf_ptr)node_map(err->node_addr);
		
		warnx("check_leaf on 0x%" PRIx32 " failed:", err->node_addr);
		
		bool have_desc = true;
		const char *err_desc;
		switch (err->code) {
		case ERR_LEAF_OVERFLOW:
			err_desc = "node content overflow";
			break;
		case ERR_LEAF_UNCONTIG:
			err_desc = "uncontiguous item data";
			break;
		case ERR_LEAF_OVERLAP:
			err_desc = "overlapping item data";
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
		case ERR_LEAF_OVERFLOW:
			warnx("%" PRIu32 " used > %" PRIu32 " possible",
				leaf_used(node), node_size_usable());
			break;
		}
		
		for (uint16_t i = 0; i < err->elem_cnt; ++i) {
			warnx("[elem %" PRIu16 "] key %s off 0x%" PRIx32 " len 0x%"PRIx32,
				err->elem_idx[i], key_str(&err->elem[i].key),
				err->elem[i].off, err->elem[i].len);
		}
		
		node_unmap((node_ptr)node);
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
