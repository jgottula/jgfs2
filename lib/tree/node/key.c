/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"


key *node_key(const node_ptr node, uint16_t idx) {
	if (idx >= node->hdr.cnt) {
		errx("%s: idx exceeds bounds: node 0x%" PRIx32 ": %" PRIu16
			" >= %" PRIu16, __func__, node->hdr.this, idx, node->hdr.cnt);
	}
	
	return &(node_elem(node, idx)->key);
}

key *node_first_key(const node_ptr node) {
	ASSERT_NONEMPTY(node);
	return &(node_elem(node, 0)->key);
}
