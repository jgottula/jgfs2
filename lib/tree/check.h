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
};

enum check_error_code {
	ERR_TREE_SORT = 1,
	ERR_TREE_DUPE = 2,
	
	ERR_NODE_SORT  = 1,
	ERR_NODE_EMPTY = 2,
	ERR_NODE_DUPE  = 3,
	
	ERR_BRANCH_KEY_MISMATCH = 1,
	
	ERR_LEAF_OVERLAP    = 1,
	ERR_LEAF_NOT_PACKED = 2,
	ERR_LEAF_WRONG_SIZE = 3,
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

struct check_result {
	uint32_t result_type;
	
	union {
		struct tree_check_error   tree_error;
		struct node_check_error   node_error;
		struct branch_check_error branch_error;
		struct leaf_check_error   leaf_error;
	};
};


struct check_result check_node_branch(branch_ptr node);
struct check_result check_node_leaf(leaf_ptr node);
struct check_result check_node(uint32_t node_addr);
struct check_result check_tree(uint32_t root_addr);


#endif
