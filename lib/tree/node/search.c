/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"


bool node_search(const node_ptr node, const key *key, uint16_t *out) {
	/* circumvent unsigned wraparound if node->hdr.cnt == 0 */
	if (node->hdr.cnt == 0) {
		return false;
	}
	
	uint16_t first = 0;
	uint16_t last  = node->hdr.cnt - 1;
	uint16_t middle;
	
	/* goal: find key that == wanted key */
	while (first <= last) {
		middle = (first + last) / 2;
		
		int8_t cmp = key_cmp(node_key(node, middle), key);
		
		if (cmp < 0) {
			first = middle + 1;
		} else if (cmp > 0) {
			last = middle - 1;
		} else {
			/* found */
			*out = middle;
			return true;
		}
	}
	
	/* not found */
	return false;
}

uint16_t node_search_hypo(const node_ptr node, const key *key) {
	/* for empty node, return first index; for largest key, return very last
	 * possible index */
	if (node->hdr.cnt == 0) {
		return 0;
	} else if (key_cmp(node_key(node, node->hdr.cnt - 1), key) < 0) {
		return node->hdr.cnt;
	}
	
	uint16_t first = 0;
	uint16_t last  = node->hdr.cnt - 1;
	uint16_t middle;
	
	/* goal: find lowest key that is > wanted key */
	while (first <= last) {
		middle = CEIL(first + last, 2);
		
		int8_t cmp = key_cmp(node_key(node, middle), key);
		
		if (cmp < 0) {
			first = middle + 1;
		} else if (cmp > 0) {
			if (middle == 0 || key_cmp(node_key(node, middle - 1), key) < 0) {
				return middle;
			} else {
				last = middle - 1;
			}
		} else {
			errx("%s: key already present: node 0x%" PRIx32 " key %s",
				__func__, node->hdr.this, key_str(key));
		}
	}
	
	errx("%s: total failure: node 0x%" PRIx32 " key %s",
		__func__, node->hdr.this, key_str(key));
}

uint32_t branch_search(const node_ptr branch, const key *key) {
	ASSERT_BRANCH(branch);
	ASSERT_NONEMPTY(branch);
	
	/* if lower than all present keys, return first node_ref */
	if (key_cmp(&branch->b_elems[0].key, key) > 0) {
		return branch->b_elems[0].addr;
	}
	
	uint16_t first = 0;
	uint16_t last  = branch->hdr.cnt - 1;
	uint16_t middle;
	
	/* goal: find highest key that is <= wanted key */
	while (first <= last) {
		middle = CEIL(first + last, 2);
		
		int8_t cmp = key_cmp(&branch->b_elems[middle].key, key);
		
		if (cmp < 0) {
			if (middle == branch->hdr.cnt - 1 ||
				key_cmp(&branch->b_elems[middle + 1].key, key) > 0) {
				return branch->b_elems[middle].addr;
			} else {
				first = middle + 1;
			}
		} else if (cmp > 0) {
			last = middle - 1;
		} else {
			return branch->b_elems[middle].addr;
		}
	}
	
	errx("%s: total failure: branch 0x%" PRIx32 " key %s",
		__func__, branch->hdr.this, key_str(key));
}

node_ref *branch_search_addr(const node_ptr branch, uint32_t addr) {
	ASSERT_BRANCH(branch);
	
	/* this is not an optimized search because the node's elements are not
	 * sorted by address */
	const node_ref *elem_end = branch->b_elems + branch->hdr.cnt;
	for (node_ref *elem = branch->b_elems; elem < elem_end; ++elem) {
		if (elem->addr == addr) {
			return elem;
		}
	}
	
	/* not found */
	return NULL;
}
