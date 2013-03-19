/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"


/// @brief gets a device mapping for a node
/// @param[in] node_addr  block number of node
/// @param[in] writable   request a read-write mapping
/// @return pointer to node
node_ptr node_map(uint32_t node_addr, bool writable) {
	return fs_map_blk(node_addr, node_size_blk(), writable);
}

/// @brief frees a node device mapping
/// @param[in] node  node pointer
void node_unmap(const node_ptr node) {
	/* msync asynchronously so we don't hurt performance too badly */
	fs_msync_blk(node, node->hdr.this, node_size_blk(), true);
	
	fs_unmap_blk(node, node->hdr.this, node_size_blk());
}
