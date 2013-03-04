/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#ifndef JGFS2_LIB_TREE_TREE_H
#define JGFS2_LIB_TREE_TREE_H


#include "../jgfs2.h"
#include "key.h"
#include "item.h"
#include "node.h"


struct tree_lock_node {
	struct tree_lock_node *next;
	
	uint32_t root_addr;
};


void tree_init(uint32_t root_addr);
void tree_dump(uint32_t root_addr);
void tree_graph(uint32_t root_addr);
leaf_ptr tree_search(uint32_t root_addr, const key *key);
bool tree_retrieve(uint32_t root_addr, const key *key, struct item_data *item);
void tree_insert(uint32_t root_addr, const key *key, struct item_data item);


#endif
