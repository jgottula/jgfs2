/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "node.h"
#include "../debug.h"


leaf_ptr leaf_init(uint32_t node_addr, uint32_t parent, uint32_t prev,
	uint32_t next) {
	leaf_ptr node = (leaf_ptr)node_map(node_addr);
	
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
		" next 0x%" PRIx32 " cnt %" PRIu16,
		__func__, hdr->this, hdr->parent, hdr->prev, hdr->next, hdr->cnt);
	
	const item_ref *elem_end = node->elems + node->hdr.cnt;
	for (const item_ref *elem = node->elems; elem < elem_end; ++elem) {
		fprintf(stderr, "        item %-4" PRIdPTR " %s len %" PRIu32 "\n",
			(elem - node->elems), key_str(&elem->key), elem->len);
		
		if (elem->len != 0) {
			dump_mem(leaf_data_ptr(node, elem), elem->len);
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
	for (uint16_t i = 0; i < node->hdr.cnt - 1; ++i) {
		used_incr += sizeof(item_ref);
		used_incr += node->elems[i].len;
		
		if (used_incr >= used_half) {
			return i + 1;
		}
	}
	
	warnx("%s: using fallback algo", __func__);
	
	/* fallback algorithm: halfway by item number */
	return node->hdr.cnt / 2;
}

void *leaf_data_ptr(const leaf_ptr node, const item_ref *item) {
	ASSERT_LEAF(node);
	
	return (uint8_t *)node + item->off;
}

void leaf_zero(leaf_ptr node, uint16_t first) {
	ASSERT_LEAF(node);
	
	const item_ref *elem = node->elems + first;
	uint8_t *data_ptr = (uint8_t *)node + elem->off;
	
	uint8_t *zero_begin = (uint8_t *)elem;
	uint8_t *zero_end   = data_ptr + elem->len;
	
	memset(zero_begin, 0, (zero_end - zero_begin));
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
	}
	
	/* the caller needs to make space if necessary */
	if (leaf_free(node) < sizeof(item_ref) + item.len) {
		return false;
	}
	
	uint16_t insert_at = node_search_hypo((node_ptr)node, key);
	
	/* shift elements above the insertion point over by one; this includes the
	 * data buffers to which they point */
	item_ref *elem_last = node->elems + (node->hdr.cnt - 1);
	for (item_ref *elem = elem_last; elem >= node->elems + insert_at; --elem) {
		item_ref *elem_next = elem + 1;
		*elem_next = *elem;
		
		uint8_t *data_ptr = (uint8_t *)node + elem_next->off;
		memmove(data_ptr - item.len, data_ptr, elem_next->len);
		
		elem_next->off -= item.len;
	}
	
	leaf_insert_naive(node, insert_at, key, item);
	++node->hdr.cnt;
	
	/* if we are not root and we just inserted the element in position 0, we
	 * need to update the node_ref to us in our parent */
	if (insert_at == 0 && node->hdr.parent != 0) {
		branch_ptr parent = (branch_ptr)node_map(node->hdr.parent);
		branch_ref_update(parent, (node_ptr)node);
		node_unmap((node_ptr)parent);
	}
	
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
	
	const item_ref *elem_end = this->elems + this->hdr.cnt;
	for (const item_ref *elem = this->elems + half; elem < elem_end; ++elem) {
		leaf_append_naive(new, &elem->key,
			(struct item_data){ elem->len, leaf_data_ptr(this, elem) });
	}
	
	leaf_zero(this, half);
	this->hdr.cnt = half;
}
