#ifndef JGFS2_LIB_TREE_TREE_H
#define JGFS2_LIB_TREE_TREE_H


#include "../jgfs2.h"
#include "common.h"
#include "key.h"
#include "item.h"
#include "node.h"


void tree_init(uint32_t root_addr);
void tree_split(uint32_t node_addr);
uint32_t tree_search(uint32_t node_addr, const key *key);


#endif
