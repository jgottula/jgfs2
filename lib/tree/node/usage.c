/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"


uint32_t node_used(const node_ptr node) {
	if (node->hdr.leaf) {
		if (node->hdr.cnt != 0) {
			uint32_t used_ref  = node->hdr.cnt * sizeof(item_ref);
			uint32_t used_data = node_size_byte() -
				node->l_elems[node->hdr.cnt - 1].off;
			
			return used_ref + used_data;
		} else {
			return 0;
		}
	} else {
		return node->hdr.cnt * sizeof(node_ref);
	}
}

uint32_t node_free(const node_ptr node) {
	return node_size_usable() - node_used(node);
}
