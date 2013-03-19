/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"


/// @brief gets the total size of an elem and its data, if any
/// @param[in] node  pointer to node
/// @param[in] idx   elem index
/// @return total elem weight in bytes
uint32_t node_elem_weight(const node_ptr node, uint16_t idx) {
	if (idx >= node->hdr.cnt) {
		errx("%s: idx >= cnt: node 0x%" PRIx32 " idx %" PRIu16 " cnt %" PRIu16,
			__func__, node->hdr.this, idx, node->hdr.cnt);
	}
	
	if (node->hdr.leaf) {
		return sizeof(item_ref) + node->l_elems[idx].len;
	} else {
		return sizeof(node_ref);
	}
}

/// @brief fills a node elem with 
/// @param[in] node     pointer to node
/// @param[in] idx      elem index
/// @param[in] key      new key
/// @param[in] payload  new payload
void node_elem_fill(node_ptr node, uint16_t idx, const key *key,
	union elem_payload payload) {
	if (node->hdr.leaf) {
		item_ref *elem = node->l_elems + idx;
		
		uint32_t off;
		if (idx == 0) {
			off = node_size_byte() - payload.l_item.len;
		} else {
			off = (elem - 1)->off - payload.l_item.len;
		}
		
		elem->key = *key;
		elem->len = payload.l_item.len;
		elem->off = off;
		
		uint8_t *data = leaf_elem_data(node, idx);
		memcpy(data, payload.l_item.data, payload.l_item.len);
	} else {
		node_ref *elem = node->b_elems + idx;
		
		elem->key  = *key;
		elem->addr = payload.b_addr;
	}
}

/// @brief gets a pointer to elem data from a leaf node pointer
/// @param[in] node  pointer to leaf node
/// @param[in] idx   elem index
/// @return pointer to elem data within the node
void *leaf_elem_data(const node_ptr leaf, uint16_t idx) {
	ASSERT_LEAF(leaf);
	
	if (idx >= leaf->hdr.cnt) {
		errx("%s: idx >= cnt: leaf 0x%" PRIx32 " idx %" PRIu16 " cnt %" PRIu16,
			__func__, leaf->hdr.this, idx, leaf->hdr.cnt);
	}
	
	return (uint8_t *)leaf + leaf->l_elems[idx].off;
}
