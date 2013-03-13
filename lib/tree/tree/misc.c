/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../tree.h"
#include "../../debug.h"


void tree_init(uint32_t root_addr) {
	tree_lock(root_addr);
	node_unmap((node_ptr)leaf_init(root_addr, 0, 0, 0));
	tree_unlock(root_addr);
}

void tree_stat(uint32_t root_addr) {
	ASSERT_ROOT(root_addr);
	
	TODO("implement this");
}
