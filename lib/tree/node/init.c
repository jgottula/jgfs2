/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"


node_ptr node_init(uint32_t node_addr, bool leaf, uint32_t parent,
	uint32_t prev, uint32_t next) {
	node_ptr node = node_map(node_addr, true);
	
	node->hdr.leaf   = leaf;
	node->hdr.cnt    = 0;
	node->hdr.this   = node_addr;
	node->hdr.prev   = prev;
	node->hdr.next   = next;
	node->hdr.parent = parent;
	
	return node;
}

node_ptr node_copy_init(uint32_t dst_addr, const node_ptr src, bool leaf,
	uint32_t parent, uint32_t prev, uint32_t next) {
	node_ptr dst = node_map(dst_addr, true);
	memcpy(dst, src, node_size_byte());
	node_unmap(dst);
	
	return node_init(dst_addr, leaf, parent, prev, next);
}
