/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"


bool node_is_root(uint32_t node_addr) {
	const node_ptr node = node_map(node_addr, false);
	bool is_root = (node->hdr.parent == 0);
	node_unmap(node);
	
	return is_root;
}

uint32_t node_find_root(uint32_t node_addr) {
	uint32_t parent_addr = node_addr;
	
	do {
		node_addr = parent_addr;
		
		node_ptr node = node_map(node_addr, false);
		parent_addr = node->hdr.parent;
		node_unmap(node);
	} while (parent_addr != 0);
	
	return node_addr;
}
