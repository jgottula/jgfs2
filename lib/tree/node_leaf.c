#include "node.h"
#include "../debug.h"


void leaf_dump(const leaf_ptr node) {
	const struct node_hdr *hdr = &node->hdr;
	
	warnx("%s: this 0x%08" PRIx32 " parent 0x%08" PRIx32 " next 0x%80" PRIx32
		" cnt %" PRIu16, __func__, hdr->this, hdr->parent, hdr->next, hdr->cnt);
	
	const item_ref *elem_last = node->elems + node->hdr.cnt;
	for (const item_ref *elem = node->elems; elem < elem_last; ++elem) {
		const key *key = &elem->key;
		
		fprintf(stderr, "item %3" PRIdPTR ": [ id %" PRIu32 " type %" PRIu8
			" off %" PRIu32 " ] len %" PRIu32 "\n", (elem - node->elems),
			key->id, key->type, key->off, elem->len);
	}
	
	dump_mem(node, sizeof(*node));
}

leaf_ptr leaf_init(uint32_t node_addr, uint32_t parent, uint32_t next) {
	leaf_ptr node = (leaf_ptr)node_map(node_addr);
	
	node->hdr.this   = node_addr;
	node->hdr.parent = parent;
	node->hdr.next   = next;
	node->hdr.cnt    = 0;
	node->hdr.leaf   = true;
	
	return node;
}

tree_result leaf_item_ptr(const leaf_ptr node, const key *key) {
	tree_result result = { NULL, false };
	
	const item_ref *elem_last = node->elems + node->hdr.cnt;
	for (const item_ref *elem = node->elems; elem < elem_last; ++elem) {
		if (key_cmp(key, &elem->key) == 0) {
			result.ptr = (uint8_t *)node + elem->off;
			result.found = true;
			break;
		}
	}
	
	return result;
}

uint32_t leaf_used(const leaf_ptr node) {
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
	return node_size_byte() - leaf_used(node);
}

uint16_t leaf_half(const leaf_ptr node) {
	uint32_t used_half = leaf_used(node) / 2;
	uint32_t used_incr = 0;
	
	/* find out how many items take up half the node space, and tell the caller
	 * to remove all the items above this halfway point */
	for (uint16_t i = 0; i < node->hdr.cnt - 1; ++i) {
		used_incr += sizeof(item_ref);
		used_incr += node->elems[i].len;
		
		if (used_incr > used_half) {
			return i + 1;
		}
	}
	
	warnx("%s: using fallback algo", __func__);
	
	/* fallback algorithm: halfway by item number */
	return node->hdr.cnt / 2;
}

void leaf_zero(leaf_ptr node, uint16_t first) {
	const item_ref *elem = node->elems + first;
	uint8_t *data_ptr = (uint8_t *)node + elem->off;
	
	uint8_t *zero_begin = (uint8_t *)elem;
	uint8_t *zero_end   = data_ptr + elem->len;
	
	memset(zero_begin, 0, (zero_end - zero_begin));
}

void leaf_append_naive(leaf_ptr node, const key *key, uint32_t len,
	const void *data) {
	uint32_t off = node->elems[node->hdr.cnt - 1].off - len;
	uint8_t *data_ptr = (uint8_t *)node + off;
	
	memcpy(data_ptr, data, len);
	
	item_ref *elem = node->elems + node->hdr.cnt;
	elem->key = *key;
	elem->off = off;
	elem->len = len;
	
	++node->hdr.cnt;
}

void leaf_xfer_half(leaf_ptr dst, leaf_ptr src) {
	uint16_t half = leaf_half(src);
	
	const item_ref *elem_last = src->elems + src->hdr.cnt;
	for (const item_ref *elem = src->elems + half; elem < elem_last; ++elem) {
		const uint8_t *data_ptr = (const uint8_t *)src + elem->off;
		leaf_append_naive(dst, &elem->key, elem->len, data_ptr);
	}
	
	leaf_zero(src, half);
}
