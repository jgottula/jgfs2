/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"


/// @brief updates the reference to this node in the parent branch
/// @param[in] node  pointer to node
void node_update_ref_in_parent(const node_ptr node) {
	/* root has no parent */
	if (node->hdr.parent == 0) {
		return;
	}
	
	node_ptr parent = node_map(node->hdr.parent, true);
	
	node_ref *this_ref = branch_search_addr(parent, node->hdr.this);
	if (this_ref == NULL) {
		errx("%s: ref not found in parent: node 0x%" PRIx32 " parent 0x%"
			PRIx32, __func__, node->hdr.this, node->hdr.parent);
	}
	
	this_ref->key = *(node_first_key(node));
	
	node_unmap(parent);
}

/// @brief 
/// @return 
/*bool node_insert(node_ptr node, const key *key,
	const union elem_payload *payload) {
	
}*/
