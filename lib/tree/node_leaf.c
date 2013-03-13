/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "node.h"
#include "../debug.h"


leaf_ptr leaf_init(uint32_t node_addr, uint32_t parent, uint32_t prev,
	uint32_t next) {
	leaf_ptr node = (leaf_ptr)node_map(node_addr, true);
	
	node->hdr.leaf   = true;
	node->hdr.cnt    = 0;
	node->hdr.this   = node_addr;
	node->hdr.prev   = prev;
	node->hdr.next   = next;
	node->hdr.parent = parent;
	
	return node;
}

void leaf_dump(const leaf_ptr node) {
	ASSERT_LEAF(node);
	
	const struct node_hdr *hdr = &node->hdr;
	warnx("%s: this 0x%" PRIx32 " parent 0x%" PRIx32 " prev 0x%" PRIx32
		" next 0x%" PRIx32 " cnt %" PRIu16 " free %" PRIu32,
		__func__, hdr->this, hdr->parent, hdr->prev, hdr->next, hdr->cnt,
		leaf_free(node));
	
	const item_ref *elem_end = node->elems + node->hdr.cnt;
	for (const item_ref *elem = node->elems; elem < elem_end; ++elem) {
		fprintf(stderr, "        item %-4" PRIdPTR " %s len %" PRIu32 "\n",
			(elem - node->elems), key_str(&elem->key), elem->len);
		
		if (elem->len != 0) {
			uint16_t idx = (elem - node->elems);
			dump_mem(leaf_data_ptr(node, idx), elem->len);
		}
	}
}

uint32_t leaf_used(const leaf_ptr node) {
	ASSERT_LEAF(node);
	
	if (node->hdr.cnt != 0) {
		uint32_t used_ref = node->hdr.cnt * sizeof(item_ref);
		
		const item_ref *last_elem = &node->elems[node->hdr.cnt - 1];
		uint32_t used_data = node_size_byte() - last_elem->off;
		
		return used_ref + used_data;
	} else {
		return 0;
	}
}

uint32_t leaf_free(const leaf_ptr node) {
	ASSERT_LEAF(node);
	
	return node_size_usable() - leaf_used(node);
}

uint16_t leaf_half(const leaf_ptr node) {
	ASSERT_LEAF(node);
	
	uint32_t used_half = CEIL(leaf_used(node), 2);
	uint32_t used_incr = 0;
	
	/* find out how many items take up half the node space, and tell the caller
	 * to remove all the items above this halfway point */
	for (uint16_t i = 0; i < node->hdr.cnt; ++i) {
		used_incr += sizeof(item_ref);
		used_incr += node->elems[node->hdr.cnt - (i + 1)].len;
		
		if (used_incr >= used_half) {
			return i;
		}
	}
	
	warnx("%s: using fallback algo", __func__);
	
	/* fallback algorithm: halfway by item number */
	return node->hdr.cnt / 2;
}

void *leaf_data_ptr(const leaf_ptr node, uint16_t idx) {
	ASSERT_LEAF(node);
	
	if (idx >= node->hdr.cnt) {
		errx("%s: idx >= cnt: node 0x%" PRIx32 " idx %" PRIu16 " cnt %" PRIu16,
			__func__, node->hdr.this, idx, node->hdr.cnt);
	}
	
	return (uint8_t *)node + node->elems[idx].off;
}

void leaf_zero(leaf_ptr node, uint16_t first) {
	ASSERT_LEAF(node);
	if (first >= node->hdr.cnt) {
		return;
	}
	
	const item_ref *elem = node->elems + first;
	
	uint8_t *zero_begin = (uint8_t *)elem;
	uint8_t *zero_end   = leaf_data_ptr(node, first) + elem->len;
	
	memset(zero_begin, 0, (zero_end - zero_begin));
}


void leaf_shift_forward(leaf_ptr node, uint16_t first, uint16_t diff_elem,
	uint32_t diff_data) {
	ASSERT_LEAF(node);
	if (first >= node->hdr.cnt) {
		return;
	}
	
	item_ref *elem_first = node->elems + first;
	item_ref *elem_last  = node->elems + (node->hdr.cnt - 1);
	
	if (diff_data != 0) {
		uint8_t *data_begin = leaf_data_ptr(node, node->hdr.cnt - 1);
		uint8_t *data_end   = leaf_data_ptr(node, first) + elem_first->len;
		
		memmove(data_begin - diff_data, data_begin, (data_end - data_begin));
	}
	
	for (item_ref *elem = elem_last; elem >= elem_first; --elem) {
		item_ref *elem_dst = elem + diff_elem;
		
		*elem_dst = *elem;
		elem_dst->off -= diff_data;
	}
}

void leaf_shift_backward(leaf_ptr node, uint16_t first, uint16_t diff_elem,
	uint32_t diff_data) {
	ASSERT_LEAF(node);
	if (first >= node->hdr.cnt) {
		return;
	}
	
	item_ref *elem_first = node->elems + first;
	item_ref *elem_last  = node->elems + (node->hdr.cnt - 1);
	
	if (diff_data != 0) {
		uint8_t *data_begin = leaf_data_ptr(node, node->hdr.cnt - 1);
		uint8_t *data_end   = leaf_data_ptr(node, first) + elem_first->len;
		
		memmove(data_begin + diff_data, data_begin, (data_end - data_begin));
	}
	
	for (item_ref *elem = elem_first; elem <= elem_last; ++elem) {
		item_ref *elem_dst = elem - diff_elem;
		
		*elem_dst = *elem;
		elem_dst->off += diff_data;
	}
}

void leaf_xfer(leaf_ptr dst, const leaf_ptr src, uint16_t dst_idx,
	uint16_t src_idx, uint16_t cnt) {
	ASSERT_LEAF(dst);
	ASSERT_LEAF(src);
	
	for (uint16_t i = 0; i < cnt; ++i) {
		const item_ref *elem_src = src->elems + (src_idx + i);
		
		leaf_insert_naive(dst, dst_idx + i, &elem_src->key,
			(struct item_data){
				.len  = elem_src->len,
				.data = leaf_data_ptr(src, src_idx + i),
			});
	}
}

void leaf_insert_naive(leaf_ptr node, uint16_t at, const key *key,
	struct item_data item) {
	ASSERT_LEAF(node);
	
	item_ref *elem = node->elems + at;
	
	uint32_t off;
	if (at == 0) {
		off = node_size_byte() - item.len;
	} else {
		item_ref *elem_prev = elem - 1;
		off = elem_prev->off - item.len;
	}
	
	uint8_t *data_ptr = (uint8_t *)node + off;
	memcpy(data_ptr, item.data, item.len);
	
	elem->key = *key;
	elem->off = off;
	elem->len = item.len;
}

void leaf_append_naive(leaf_ptr node, const key *key, struct item_data item) {
	ASSERT_LEAF(node);
	
	leaf_insert_naive(node, node->hdr.cnt, key, item);
	++node->hdr.cnt;
}

bool leaf_insert(leaf_ptr node, const key *key, struct item_data item) {
	ASSERT_LEAF(node);
	
	if (sizeof(item_ref) + item.len > node_size_usable()) {
		errx("%s: will never fit: node 0x%" PRIx32 " %s len %" PRIu32,
			__func__, node->hdr.this, key_str(key), item.len);
	} else if (item.len * 10 >= node_size_usable()) {
		warnx("%s: obese item: node 0x%" PRIx32 " %s len %" PRIu32,
			__func__, node->hdr.this, key_str(key), item.len);
	}
	
	/* the caller needs to make space if necessary */
	if (leaf_free(node) < sizeof(item_ref) + item.len) {
		return false;
	}
	
	uint16_t insert_at = node_search_hypo((node_ptr)node, key);
	
	leaf_shift_forward(node, insert_at, 1, item.len);
	leaf_insert_naive(node, insert_at, key, item);
	++node->hdr.cnt;
	
	/* if we are not root and we just inserted the element in position 0, we
	 * need to update the node_ref to us in our parent */
	if (insert_at == 0 && node->hdr.parent != 0) {
		branch_ptr parent = (branch_ptr)node_map(node->hdr.parent, true);
		branch_ref_update(parent, (node_ptr)node);
		node_unmap((node_ptr)parent);
	}
	
	return true;
}

bool leaf_remove(leaf_ptr node, const key *key) {
	ASSERT_LEAF(node);
	
	uint16_t remove_at;
	if (!node_search((node_ptr)node, key, &remove_at)) {
		return false;
	}
	
	TODO("test data shift");
	TODO("test zero");
	
	uint32_t data_len = node->elems[remove_at].len;
	
	leaf_shift_backward(node, remove_at + 1, 1, data_len);
	leaf_zero(node, node->hdr.cnt - 1);
	--node->hdr.cnt;
	
	/* if we are not root and we just removed the element in position 0, we
	 * need to update the node_ref to us in our parent */
	if (remove_at == 0 && node->hdr.parent != 0) {
		branch_ptr parent = (branch_ptr)node_map(node->hdr.parent, true);
		branch_ref_update(parent, (node_ptr)node);
		node_unmap((node_ptr)parent);
	}
	
	node_merge(node->hdr.this);
	
	return true;
}

void leaf_split_post(leaf_ptr this, leaf_ptr new) {
	ASSERT_LEAF(this);
	ASSERT_LEAF(new);
	
	uint16_t half = leaf_half(this);
	
	if (half == 0) {
		errx("%s: half == 0", __func__);
	} else if (half == this->hdr.cnt) {
		errx("%s: half == this->hdr.cnt", __func__);
	}
	
	leaf_xfer(new, this, 0, half, this->hdr.cnt - half);
	new->hdr.cnt = this->hdr.cnt - half;
	
	leaf_zero(this, half);
	this->hdr.cnt = half;
}
