#include "node.h"
#include <inttypes.h>
#include "../debug.h"


void branch_dump(const branch_ptr node) {
	const struct node_hdr *hdr = &node->hdr;
	
	warnx("%s: this 0x%08" PRIx32 " parent 0x%08" PRIx32 " cnt %" PRIu16,
		__func__, hdr->this, hdr->parent, hdr->cnt);
	
	const node_ref *elem_last = node->elems + node->hdr.cnt;
	for (const node_ref *elem = node->elems; elem < elem_last; ++elem) {
		const key *key = &elem->key;
		
		fprintf(stderr, "node %3" PRIdPTR ": [ id %" PRIu32 " type %" PRIu8
			" off %" PRIu32 "] addr 0x%08" PRIx32 "\n", (elem - node->elems),
			key->id, key->type, key->off, elem->addr);
	}
	
	dump_mem(node, sizeof(*node));
}

branch_ptr branch_init(uint32_t node_addr, uint32_t parent) {
	branch_ptr node = (branch_ptr)node_map(node_addr);
	
	node->hdr.this   = node_addr;
	node->hdr.parent = parent;
	node->hdr.next   = 0;
	node->hdr.cnt    = 0;
	node->hdr.leaf   = false;
	
	return node;
}

uint32_t branch_used(const branch_ptr node) {
	return node->hdr.cnt * sizeof(node_ref);
}

uint32_t branch_free(const branch_ptr node) {
	return node_size_byte() - branch_used(node);
}

uint16_t branch_half(const branch_ptr node) {
	return node->hdr.cnt / 2;
}

void branch_zero(branch_ptr node, uint16_t first) {
	const node_ref *elem = node->elems + first;
	
	uint8_t *zero_begin = (uint8_t *)elem;
	uint8_t *zero_end   = (uint8_t *)node + node_size_byte();
	
	memset(zero_begin, 0, (zero_end - zero_begin));
}

void branch_append_naive(branch_ptr node, const node_ref *elem) {
	node->elems[node->hdr.cnt++] = *elem;
}

void branch_xfer_half(branch_ptr dst, branch_ptr src) {
	uint16_t half = branch_half(src);
	
	const node_ref *elem_last = src->elems + src->hdr.cnt;
	for (const node_ref *elem = src->elems + half; elem < elem_last; ++elem) {
		branch_append_naive(dst, elem);
	}
	
	branch_zero(src, half);
}
