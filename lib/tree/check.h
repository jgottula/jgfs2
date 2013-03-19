/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#ifndef JGFS2_LIB_TREE_CHECK_H
#define JGFS2_LIB_TREE_CHECK_H


#include "../jgfs2.h"
#include "key.h"
#include "item.h"
#include "node.h"
#include "tree.h"


enum check_result_type {
	RESULT_TYPE_OK     = 0,
	RESULT_TYPE_TREE   = 1,
	RESULT_TYPE_NODE   = 2,
	RESULT_TYPE_BRANCH = 3,
	RESULT_TYPE_LEAF   = 4,
	RESULT_TYPE_ITEM   = 5,
};

enum check_error_code {
	ERR_TREE_SORT       = 1,    // this[last] > next[first]
	ERR_TREE_DUPE       = 2,    // this[last] == next[first]
	ERR_TREE_NEXT_SKIP  = 3,    // next->next skips a node
	ERR_TREE_NEXT_ORDER = 4,    // next->next goes backwards
	ERR_TREE_PREV_SKIP  = 5,    // prev->prev skips a node
	ERR_TREE_PREV_ORDER = 6,    // prev->prev goes forwards
	
	ERR_NODE_THIS     = 0,      // hdr.this is wrong
	ERR_NODE_EMPTY    = 1,      // !root and no elems
	ERR_NODE_OVERFLOW = 2,      // too much item data
	ERR_NODE_SORT     = 3,      // key[0] > key[1]
	ERR_NODE_DUPE     = 4,      // key @ elem_idx[0] == key @ elem_idx[1]
	#warning generalize prev/next and depth checks to all nodes
	
	ERR_BRANCH_PARENT      = 1, // hdr.this != child.parent
	ERR_BRANCH_EMPTY_CHILD = 2, // child.hdr.cnt == 0
	ERR_BRANCH_KEY         = 3, // elem.key != child.keys[0]
	
	ERR_LEAF_PREV_BRANCH = 1,   // prev points to a branch
	ERR_LEAF_NEXT_BRANCH = 2,   // next points to a branch
	ERR_LEAF_UNCONTIG    = 3,   // wasted space between item data
	ERR_LEAF_OVERLAP     = 4,   // elem data regions overlap
	
	ERR_ITEM_TYPE    = 1,       // invalid item type
	ERR_ITEM_KEY_ID  = 2,       // inappropriate key id for item type
	ERR_ITEM_KEY_OFF = 3,       // inappropriate key off for item type
	ERR_ITEM_SIZE    = 4,       // inappropriate size for item type
};


struct tree_check_error {
	uint32_t code;
	uint32_t root_addr;
	
	uint32_t node_addr;
	key      key;
};

struct node_check_error {
	uint32_t code;
	uint32_t node_addr;
	
	uint16_t elem_cnt;
	uint16_t elem_idx[2];
	key      key[2];
};

struct branch_check_error {
	uint32_t code;
	uint32_t branch_addr;
	
	uint16_t elem_cnt;
	uint16_t elem_idx;
	node_ref elem;
};

struct leaf_check_error {
	uint32_t code;
	uint32_t leaf_addr;
	
	uint16_t elem_cnt;
	uint16_t elem_idx[2];
	item_ref elem[2];
};

struct item_check_error {
	uint32_t code;
	
	key key;
	struct item_data item;
};

struct check_result {
	uint32_t type;
	
	union {
		struct tree_check_error   tree;
		struct node_check_error   node;
		struct branch_check_error branch;
		struct leaf_check_error   leaf;
		struct item_check_error   item;
	};
};


/* common code */
void check_print(struct check_result result, bool fatal);

/* tree checking */
struct check_result check_tree(uint32_t root_addr);

/* node checking */
struct check_result check_node(uint32_t node_addr, bool recurse);

/* branch checking */
struct check_result check_branch(const node_ptr branch);

/* leaf checking */
struct check_result check_leaf(const node_ptr leaf);

/* item checking */
struct check_result check_item(const key *key, struct item_data item);


#endif
