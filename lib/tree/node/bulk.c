/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"


/// @brief zeroes the entire node, except the header
/// @param[in] node  pointer to node
void node_zero_all(node_ptr node) {
	uint8_t *zero_begin = (uint8_t *)node + sizeof(struct node_hdr);
	uint8_t *zero_end   = (uint8_t *)node + node_size_byte();
	
	memset(zero_begin, 0, (zero_end - zero_begin));
}

/// @brief zeroes all elems in a node, starting from a particular index
/// @param[in] node   pointer to node
/// @param[in] first  first index to zero
void node_zero_range(node_ptr node, uint16_t first) {
	if (first >= node->hdr.cnt) {
		errx("%s: first exceeds bounds: node 0x%" PRIx32 ": %" PRIu16
			" >= %" PRIu16, __func__, node->hdr.this, first, node->hdr.cnt);
	}
	
	uint8_t *zero_begin;
	uint8_t *zero_end;
	
	if (node->hdr.leaf) {
		const item_ref *elem_first = node->l_elems + first;
		
		zero_begin = (uint8_t *)elem_first;
		zero_end   = (uint8_t *)leaf_elem_data(node, first) + elem_first->len;
	} else {
		const node_ref *elem_first = node->b_elems + first;
		
		zero_begin = (uint8_t *)elem_first;
		zero_end   = (uint8_t *)node + node_size_byte();
	}
	
	memset(zero_begin, 0, (zero_end - zero_begin));
}

/// @brief shifts all elems in a node to a higher index (and offset)
/// @param[in] node       pointer to node
/// @param[in] first      first index to shift
/// @param[in] last       last index to shift
/// @param[in] diff_elem  amount by which to shift elems
/// @param[in] diff_data  amount by which to shift data offsets (if leaf node)
void node_shift_forward(node_ptr node, uint16_t first, uint16_t last,
	uint16_t diff_elem, uint32_t diff_data) {
	if (first > last) {
		errx("%s: first > last: node 0x%" PRIx32 ": %" PRIu16
			" >= %" PRIu16, __func__, node->hdr.this, first, last);
	} else if (last >= node->hdr.cnt) {
		errx("%s: last exceeds bounds: node 0x%" PRIx32 ": %" PRIu16
			" >= %" PRIu16, __func__, node->hdr.this, last, node->hdr.cnt);
	} else if (UINT16_MAX - diff_elem < last) {
		errx("%s: will exceed bounds: node 0x%" PRIx32 ": %" PRIu16 " + %"
			PRIu16 " >= %" PRIu16,
			__func__, node->hdr.this, last, diff_elem, node->hdr.cnt);
	}
	
	if (diff_data != 0) {
		ASSERT_LEAF(node);
		
		uint8_t *data_begin = leaf_elem_data(node, last);
		uint8_t *data_end   = leaf_elem_data(node, first) +
			node->l_elems[first].len;
		
		memmove(data_begin - diff_data, data_begin, (data_end - data_begin));
	}
	
	if (node->hdr.leaf) {
		item_ref *elem_first = node->l_elems + first;
		item_ref *elem_last  = node->l_elems + last;
		
		for (item_ref *elem = elem_last; elem >= elem_first; --elem) {
			item_ref *elem_dst = elem + diff_elem;
			
			*elem_dst = *elem;
			elem_dst->off -= diff_data;
		}
	} else {
		node_ref *elem_first = node->b_elems + first;
		node_ref *elem_last  = node->b_elems + last;
		
		for (node_ref *elem = elem_last; elem >= elem_first; --elem) {
			node_ref *elem_dst = elem + diff_elem;
			
			*elem_dst = *elem;
		}
	}
}

/// @brief shifts all elems in a node to a lower index (and offset)
/// @param[in] node       pointer to node
/// @param[in] first      first index to shift
/// @param[in] last       last index to shift
/// @param[in] diff_elem  amount by which to shift elems
/// @param[in] diff_data  amount by which to shift data offsets (if leaf node)
void node_shift_backward(node_ptr node, uint16_t first, uint16_t last,
	uint16_t diff_elem, uint32_t diff_data) {
	if (first > last) {
		errx("%s: first > last: node 0x%" PRIx32 ": %" PRIu16
			" >= %" PRIu16, __func__, node->hdr.this, first, last);
	} else if (last >= node->hdr.cnt) {
		errx("%s: last exceeds bounds: node 0x%" PRIx32 ": %" PRIu16
			" >= %" PRIu16, __func__, node->hdr.this, last, node->hdr.cnt);
	} else if (first < diff_elem) {
		errx("%s: will exceed bounds: node 0x%" PRIx32 ": %" PRIu16 " - %"
			PRIu16 " < 0", __func__, node->hdr.this, first, diff_elem);
	}
	
	if (diff_data != 0) {
		ASSERT_LEAF(node);
		
		uint8_t *data_begin = leaf_elem_data(node, last);
		uint8_t *data_end   = leaf_elem_data(node, first) +
			node->l_elems[first].len;
		
		memmove(data_begin + diff_data, data_begin, (data_end - data_begin));
	}
	
	if (node->hdr.leaf) {
		item_ref *elem_first = node->l_elems + first;
		item_ref *elem_last  = node->l_elems + last;
		
		for (item_ref *elem = elem_first; elem <= elem_last; ++elem) {
			item_ref *elem_dst = elem - diff_elem;
			
			*elem_dst = *elem;
			elem_dst->off += diff_data;
		}
	} else {
		node_ref *elem_first = node->b_elems + first;
		node_ref *elem_last  = node->b_elems + last;
		
		for (node_ref *elem = elem_first; elem <= elem_last; ++elem) {
			node_ref *elem_dst = elem - diff_elem;
			
			*elem_dst = *elem;
		}
	}
}

/// @brief copies a range of elems from one node to another
/// @param[in] dst       pointer to destination node
/// @param[in] src       pointer to source node
/// @param[in] dst_idx   first index in destination to copy to
/// @param[in] src_idx   first index in source to copy from
/// @param[in] elem_cnt  number of elements to copy
/// @param[in] data_len  total length of element data in the range, if any
static void node_xfer_multiple(node_ptr dst, const node_ptr src,
	uint16_t dst_idx, uint16_t src_idx, uint16_t elem_cnt, uint32_t data_len) {
	if (src_idx + elem_cnt > src->hdr.cnt) {
		errx("%s: [%" PRIu16 ", %" PRIu16 ") > %" PRIu16 ": dst 0x%" PRIx32
			" src 0x%" PRIx32 " src_idx %" PRIu16 " dst_idx %" PRIu16
			" elem_cnt %" PRIu16 " data_len %" PRIu32,
			__func__, src_idx, src_idx + elem_cnt, src->hdr.cnt, dst->hdr.this,
			src->hdr.this, src_idx, dst_idx, elem_cnt, data_len);
	}
	
	if (dst->hdr.leaf) {
		ASSERT_LEAF(src);
		
		uint32_t dst_off;
		if (dst->hdr.cnt == 0) {
			dst_off = node_size_byte();
		} else {
			dst_off = dst->l_elems[dst_idx].off;
		}
		
		item_ref *elem_dst       = dst->l_elems + dst_idx;
		const item_ref *elem_src = src->l_elems + src_idx;
		while (elem_cnt-- != 0) {
			*elem_dst = *elem_src;
			
			dst_off -= elem_src->len;
			elem_dst->off = dst_off;
			
			++elem_dst;
			++elem_src;
		}
		
		uint8_t       *data_dst = (uint8_t *)dst + dst_off;
		const uint8_t *data_src = leaf_elem_data(src, src_idx + (elem_cnt - 1));
		
		memcpy(data_dst, data_src, data_len);
	} else {
		ASSERT_BRANCH(src);
		
		node_ref *elem_dst       = dst->b_elems + dst_idx;
		const node_ref *elem_src = src->b_elems + src_idx;
		while (elem_cnt-- != 0) {
			*elem_dst = *elem_src;
			
			++elem_dst;
			++elem_src;
		}
	}
}

/// @brief copies a range of elems from one node to the end of another
/// @param[in] dst       pointer to destination node
/// @param[in] src       pointer to source node
/// @param[in] src_idx   first index in source to copy from
/// @param[in] elem_cnt  number of elements to copy
/// @param[in] data_len  total length of element data in the range, if any
void node_append_multiple(node_ptr dst, const node_ptr src, uint16_t src_idx,
	uint16_t elem_cnt, uint32_t data_len) {
	uint16_t dst_idx = dst->hdr.cnt;
	node_xfer_multiple(dst, src, dst_idx, src_idx, elem_cnt, data_len);
	dst->hdr.cnt += elem_cnt;
}

/// @brief copies a range of elems from one node to the beginning of another
/// @param[in] dst       pointer to destination node
/// @param[in] src       pointer to source node
/// @param[in] src_idx   first index in source to copy from
/// @param[in] elem_cnt  number of elements to copy
/// @param[in] data_len  total length of element data in the range, if any
void node_prepend_multiple(node_ptr dst, const node_ptr src, uint16_t src_idx,
	uint16_t elem_cnt, uint32_t data_len) {
	node_shift_forward(dst, 0, dst->hdr.cnt - 1, elem_cnt, data_len);
	
	uint16_t dst_idx = 0;
	node_xfer_multiple(dst, src, dst_idx, src_idx, elem_cnt, data_len);
	dst->hdr.cnt += elem_cnt;
}
