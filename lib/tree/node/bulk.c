/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"


void node_zero_all(node_ptr node) {
	uint8_t *zero_begin = (uint8_t *)node + sizeof(struct node_hdr);
	uint8_t *zero_end   = (uint8_t *)node + node_size_byte();
	
	memset(zero_begin, 0, (zero_end - zero_begin));
}

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

void node_shift_forward(node_ptr node, uint16_t first, uint16_t diff_elem,
	uint32_t diff_data) {
	if (first >= node->hdr.cnt) {
		errx("%s: first exceeds bounds: node 0x%" PRIx32 ": %" PRIu16
			" >= %" PRIu16, __func__, node->hdr.this, first, node->hdr.cnt);
	} else if (UINT16_MAX - diff_elem < first) {
		errx("%s: will exceed bounds: node 0x%" PRIx32 ": %" PRIu16 " + %"
			PRIu16 " >= %" PRIu16,
			__func__, node->hdr.this, first, diff_elem, node->hdr.cnt);
	}
	
	if (diff_data != 0) {
		ASSERT_LEAF(node);
		
		uint8_t *data_begin = leaf_elem_data(node, node->hdr.cnt - 1);
		uint8_t *data_end   = leaf_elem_data(node, first) +
			node->l_elems[first].len;
		
		memmove(data_begin - diff_data, data_begin, (data_end - data_begin));
	}
	
	if (node->hdr.leaf) {
		item_ref *elem_first = node->l_elems + first;
		item_ref *elem_last  = node->l_elems + (node->hdr.cnt - 1);
		
		for (item_ref *elem = elem_last; elem >= elem_first; --elem) {
			item_ref *elem_dst = elem + diff_elem;
			
			*elem_dst = *elem;
			elem_dst->off -= diff_data;
		}
	} else {
		node_ref *elem_first = node->b_elems + first;
		node_ref *elem_last  = node->b_elems + (node->hdr.cnt - 1);
		
		for (node_ref *elem = elem_last; elem >= elem_first; --elem) {
			node_ref *elem_dst = elem + diff_elem;
			
			*elem_dst = *elem;
		}
	}
}

void node_shift_backward(node_ptr node, uint16_t first, uint16_t diff_elem,
	uint32_t diff_data) {
	if (first >= node->hdr.cnt) {
		errx("%s: first exceeds bounds: node 0x%" PRIx32 ": %" PRIu16
			" >= %" PRIu16, __func__, node->hdr.this, first, node->hdr.cnt);
	} else if (first < diff_elem) {
		errx("%s: will exceed bounds: node 0x%" PRIx32 ": %" PRIu16 " - %"
			PRIu16 " < 0", __func__, node->hdr.this, first, diff_elem);
	}
	
	if (diff_data != 0) {
		ASSERT_LEAF(node);
		
		uint8_t *data_begin = leaf_elem_data(node, node->hdr.cnt - 1);
		uint8_t *data_end   = leaf_elem_data(node, first) +
			node->l_elems[first].len;
		
		memmove(data_begin + diff_data, data_begin, (data_end - data_begin));
	}
	
	if (node->hdr.leaf) {
		item_ref *elem_first = node->l_elems + first;
		item_ref *elem_last  = node->l_elems + (node->hdr.cnt - 1);
		
		for (item_ref *elem = elem_first; elem <= elem_last; ++elem) {
			item_ref *elem_dst = elem - diff_elem;
			
			*elem_dst = *elem;
			elem_dst->off += diff_data;
		}
	} else {
		node_ref *elem_first = node->b_elems + first;
		node_ref *elem_last  = node->b_elems + (node->hdr.cnt - 1);
		
		for (node_ref *elem = elem_first; elem <= elem_last; ++elem) {
			node_ref *elem_dst = elem - diff_elem;
			
			*elem_dst = *elem;
		}
	}
}

void node_append_multiple(node_ptr dst, const node_ptr src, uint16_t src_idx,
	uint16_t elem_cnt, uint32_t data_len) {
	if (src_idx + elem_cnt > src->hdr.cnt) {
		errx("%s: [%" PRIu16 ", %" PRIu16 ") > %" PRIu16 ": dst 0x%" PRIx32
			" src 0x%" PRIx32 " src_idx %" PRIu16 " elem_cnt %" PRIu16
			" data_len %" PRIu32,
			__func__, src_idx, src_idx + elem_cnt, src->hdr.cnt, dst->hdr.this,
			src->hdr.this, src_idx, elem_cnt, data_len);
	}
	
	uint16_t dst_idx = dst->hdr.cnt;
	
	if (dst->hdr.leaf) {
		ASSERT_LEAF(src);
		
		uint32_t off;
		if (dst->hdr.cnt == 0) {
			off = node_size_byte();
		} else {
			const item_ref *elem_last = dst->l_elems + (dst->hdr.cnt - 1);
			off = elem_last->off;
		}
		
		item_ref *elem_dst       = dst->l_elems + dst_idx;
		const item_ref *elem_src = src->l_elems + src_idx;
		while (elem_cnt-- != 0) {
			*elem_dst = *elem_src;
			
			off -= elem_src->len;
			elem_dst->off = off;
			
			++elem_dst;
			++elem_src;
		}
		
		uint8_t *data_dst = off;
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
	
	dst->hdr.cnt += elem_cnt;
}

void node_prepend_multiple() {
	
}



#warning get rid of node_xfer
#if 0
void node_xfer(node_ptr dst, const node_ptr src, uint16_t dst_idx,
	uint16_t src_idx, uint16_t cnt) {
	if (dst->hdr.leaf) {
		ASSERT_LEAF(src);
		
		#warning
		/*for (uint16_t i = 0; i < cnt; ++i) {
		const item_ref *elem_src = src->elems + (src_idx + i);
		
		leaf_insert_naive(dst, dst_idx + i, &elem_src->key,
			(struct item_data){
				.len  = elem_src->len,
				.data = leaf_data_ptr(src, src_idx + i),
			});
		}*/
	} else {
		ASSERT_BRANCH(src);
		
		#warning
		for (uint16_t i = 0; i < cnt; ++i) {
			branch_insert_naive(dst, dst_idx + i, src->b_elems + (src_idx + i));
		}
	}
}
#endif
