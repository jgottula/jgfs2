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
	ERR_TREE_SORT       = 1, // key[n] > key[n+1]
	ERR_TREE_DUPE       = 2, // key[a] == key[b]
	ERR_TREE_NEXT_SKIP  = 3, // next->next skips a node
	ERR_TREE_NEXT_ORDER = 4, // next->next goes backwards
	
	ERR_NODE_SORT  = 1,      // key[n] > key[n+1]
	ERR_NODE_EMPTY = 2,      // !root and no elems
	ERR_NODE_DUPE  = 3,      // key[a] == key[b]
	
	ERR_BRANCH_KEY      = 1, // elem.key != child.keys[0]
	ERR_BRANCH_PARENT   = 2, // elem.this != child.parent
	ERR_BRANCH_OVERFLOW = 3, // cnt items wouldn't fit in a node
	
	ERR_LEAF_OVERLAP  = 1,   // elem data regions overlap
	ERR_LEAF_UNCONTIG = 2,   // wasted space between item data
	ERR_LEAF_OVERFLOW = 3,   // item_ref overlaps item data
	
	ERR_ITEM_KEY  = 1,       // inappropriate id or off for item type
	ERR_ITEM_SIZE = 2,       // inappropriate size for item type
};

struct tree_check_error {
	uint32_t error_code;
	
	uint32_t root_addr;
	uint32_t node_addr;
	key key;
};

struct node_check_error {
	uint32_t error_code;
	
	uint32_t node_addr;
	key key;
};

struct branch_check_error {
	uint32_t error_code;
	
	uint32_t node_addr;
	node_ref elem;
};

struct leaf_check_error {
	uint32_t error_code;
	
	uint32_t node_addr;
	item_ref elem;
};

struct item_check_error {
	uint32_t error_code;
	
	key key;
	struct item_data item;
};

struct check_result {
	uint32_t result_type;
	
	union {
		struct tree_check_error   tree_error;
		struct node_check_error   node_error;
		struct branch_check_error branch_error;
		struct leaf_check_error   leaf_error;
		struct item_check_error   item_error;
	};
};


struct check_result check_item(const key *key, struct item_data item);
struct check_result check_node_branch(branch_ptr node);
struct check_result check_node_leaf(leaf_ptr node);
struct check_result check_node(uint32_t node_addr);
struct check_result check_tree(uint32_t root_addr);


#endif
