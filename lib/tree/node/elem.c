/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"


void *leaf_elem_data(const node_ptr leaf, uint16_t idx) {
	ASSERT_LEAF(leaf);
	
	if (idx >= leaf->hdr.cnt) {
		errx("%s: idx >= cnt: leaf 0x%" PRIx32 " idx %" PRIu16 " cnt %" PRIu16,
			__func__, leaf->hdr.this, idx, leaf->hdr.cnt);
	}
	
	return (uint8_t *)leaf + leaf->l_elems[idx].off;
}
