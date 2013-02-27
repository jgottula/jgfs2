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


void tree_dump(uint32_t root_addr);
void tree_init(uint32_t root_addr);
void tree_insert(uint32_t root_addr, const key *key, struct item_data item);
leaf_ptr tree_search(uint32_t root_addr, const key *key);
uint32_t tree_find_root(uint32_t node_addr);


#endif
