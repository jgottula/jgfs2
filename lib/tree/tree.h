#ifndef JGFS2_LIB_TREE_TREE_H
#define JGFS2_LIB_TREE_TREE_H


#include "../jgfs2.h"
#include "common.h"
#include "key.h"
#include "item.h"
#include "node.h"


void tree_init(uint32_t root_addr);
void tree_insert(uint32_t root_addr, const key *key, struct item_data item);
leaf_ptr tree_search(uint32_t root_addr, const key *key);


#endif
