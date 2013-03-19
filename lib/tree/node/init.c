/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"


/// @brief initializes a node
/// @param[in] node_addr  block number of new node
/// @param[in] leaf       initialize as a leaf node
/// @param[in] parent     block number of parent node
/// @param[in] prev       block number of left sibling node
/// @param[in] next       block number of right sibling node
/// @return device-mapped pointer to new node
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

/// @brief initializes a node with the elems and data of an existing node
/// @param[in] dst_addr  block number of new node
/// @param[in] src       pointer to existing node
/// @param[in] parent    block number of parent node
/// @param[in] prev      block number of left sibling node
/// @param[in] next      block number of right sibling node
/// @return device-mapped pointer to new node
node_ptr node_copy_init(uint32_t dst_addr, const node_ptr src, uint32_t parent,
	uint32_t prev, uint32_t next) {
	node_ptr dst = node_map(dst_addr, true);
	memcpy(dst, src, node_size_byte());
	node_unmap(dst);
	
	node_ptr node = node_init(dst_addr, src->hdr.leaf, parent, prev, next);
	node->hdr.cnt = src->hdr.cnt;
	return node;
}
