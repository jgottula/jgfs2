/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"


static void node_dump_branch(node_ptr branch) {
	warnx("%s: this 0x%" PRIx32 " parent 0x%" PRIx32 " cnt %" PRIu16 " free %"
		PRIu32, __func__, branch->hdr.this, branch->hdr.parent, branch->hdr.cnt,
		node_free(branch));
	
	#warning
	const node_ref *elem_end = branch->b_elems + branch->hdr.cnt;
	for (const node_ref *elem = branch->b_elems; elem < elem_end; ++elem) {
		fprintf(stderr, "        branch %-4" PRIdPTR " %s addr 0x%" PRIx32 "\n",
			(elem - branch->b_elems), key_str(&elem->key), elem->addr);
	}
}

static void node_dump_leaf(node_ptr leaf) {
	warnx("%s: this 0x%" PRIx32 " parent 0x%" PRIx32 " prev 0x%" PRIx32
		" next 0x%" PRIx32 " cnt %" PRIu16 " free %" PRIu32,
		__func__, leaf->hdr.this, leaf->hdr.parent, leaf->hdr.prev,
		leaf->hdr.next, leaf->hdr.cnt, node_free(leaf));
	
	#warning
	const item_ref *elem_end = leaf->l_elems + leaf->hdr.cnt;
	for (const item_ref *elem = leaf->l_elems; elem < elem_end; ++elem) {
		fprintf(stderr, "        item %-4" PRIdPTR " %s len %" PRIu32 "\n",
			(elem - leaf->l_elems), key_str(&elem->key), elem->len);
		
		if (elem->len != 0) {
			uint16_t idx = (elem - leaf->l_elems);
			dump_mem(leaf_elem_data(leaf, idx), elem->len);
		}
	}
}

void node_dump(uint32_t node_addr, bool recurse) {
	node_ptr node = node_map(node_addr, false);
	
	if (node->hdr.leaf) {
		node_dump_leaf(node);
	} else {
		node_dump_branch(node);
		
		if (recurse) {
			const node_ref *elem_end = node->b_elems + node->hdr.cnt;
			for (const node_ref *elem = node->b_elems;
				elem < elem_end; ++elem) {
				node_dump(elem->addr, true);
			}
		}
	}
	
	node_unmap(node);
}
