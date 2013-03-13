/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../check.h"
#include "../../debug.h"


void check_print(struct check_result result, bool fatal) {
	if (result.type == RESULT_TYPE_OK) {
		return;
	} else if (result.type == RESULT_TYPE_TREE) {
		//const struct tree_check_error *err = &result.tree;
		
		TODO("report for tree results");
	} else if (result.type == RESULT_TYPE_NODE) {
		const struct node_check_error *err = &result.node;
		const node_ptr node = node_map(err->node_addr, false);
		
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
			warnx("unknown error (%" PRIu32 ")", err->code);
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
		const branch_ptr node = (branch_ptr)node_map(err->node_addr, false);
		
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
			warnx("unknown error (%" PRIu32 ")", err->code);
			err_desc = "unknown error";
		}
		
		node_ptr child;
		if (err->elem_cnt > 0) {
			child = node_map(err->elem.addr, false);
			
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
			warnx("actual first key: %s", key_str(node_first_key(child)));
			break;
		}
		
		if (err->elem_cnt > 0) {
			node_unmap(child);
		}
		node_unmap((node_ptr)node);
	} else if (result.type == RESULT_TYPE_LEAF) {
		const struct leaf_check_error *err = &result.leaf;
		const leaf_ptr node = (leaf_ptr)node_map(err->node_addr, false);
		
		warnx("check_leaf on 0x%" PRIx32 " failed:", err->node_addr);
		
		bool have_desc = true;
		const char *err_desc;
		switch (err->code) {
		case ERR_LEAF_PREV_BRANCH:
			err_desc = "prev points to branch";
			break;
		case ERR_LEAF_NEXT_BRANCH:
			err_desc = "next points to branch";
			break;
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
			warnx("unknown error (%" PRIu32 ")", err->code);
			err_desc = "unknown error";
		}
		
		switch (err->code) {
		case ERR_LEAF_PREV_BRANCH:
			warnx("prev 0x%" PRIx32, node->hdr.prev);
			break;
		case ERR_LEAF_NEXT_BRANCH:
			warnx("next 0x%" PRIx32, node->hdr.next);
			break;
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
