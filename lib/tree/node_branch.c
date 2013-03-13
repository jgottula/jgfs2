/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "node.h"
#include "../debug.h"


branch_ptr branch_init(uint32_t node_addr, uint32_t parent) {
	branch_ptr node = (branch_ptr)node_map(node_addr, true);
	
	node->hdr.leaf   = false;
	node->hdr.cnt    = 0;
	node->hdr.this   = node_addr;
	node->hdr.prev   = 0;
	node->hdr.next   = 0;
	node->hdr.parent = parent;
	
	return node;
}

void branch_dump(const branch_ptr node) {
	ASSERT_BRANCH(node);
	
	const struct node_hdr *hdr = &node->hdr;
	warnx("%s: this 0x%" PRIx32 " parent 0x%" PRIx32 " cnt %" PRIu16 " free %"
		PRIu32, __func__, hdr->this, hdr->parent, hdr->cnt, branch_free(node));
	
	const node_ref *elem_end = node->elems + node->hdr.cnt;
	for (const node_ref *elem = node->elems; elem < elem_end; ++elem) {
		fprintf(stderr, "        node %-4" PRIdPTR " %s addr 0x%" PRIx32 "\n",
			(elem - node->elems), key_str(&elem->key), elem->addr);
	}
}

uint32_t branch_used(const branch_ptr node) {
	ASSERT_BRANCH(node);
	
	return node->hdr.cnt * sizeof(node_ref);
}

uint32_t branch_free(const branch_ptr node) {
	ASSERT_BRANCH(node);
	
	return node_size_usable() - branch_used(node);
}

uint16_t branch_half(const branch_ptr node) {
	ASSERT_BRANCH(node);
	
	return node->hdr.cnt / 2;
}

uint32_t branch_search(const branch_ptr node, const key *key) {
	ASSERT_BRANCH(node);
	ASSERT_NONEMPTY(node);
	
	/* if lower than all present keys, return first node_ref */
	if (key_cmp(&node->elems[0].key, key) > 0) {
		return node->elems[0].addr;
	}
	
	uint16_t first = 0;
	uint16_t last  = node->hdr.cnt - 1;
	uint16_t middle;
	
	/* goal: find highest key that is <= wanted key */
	while (first <= last) {
		middle = CEIL(first + last, 2);
		
		int8_t cmp = key_cmp(&node->elems[middle].key, key);
		
		if (cmp < 0) {
			if (middle == node->hdr.cnt - 1 ||
				key_cmp(&node->elems[middle + 1].key, key) > 0) {
				return node->elems[middle].addr;
			} else {
				first = middle + 1;
			}
		} else if (cmp > 0) {
			last = middle - 1;
		} else {
			return node->elems[middle].addr;
		}
	}
	
	errx("%s: total failure: node 0x%" PRIx32 " key %s",
		__func__, node->hdr.this, key_str(key));
}

node_ref *branch_search_addr(const branch_ptr node, uint32_t addr) {
	ASSERT_BRANCH(node);
	
	/* this is not an optimized search because the node's elements are not
	 * sorted by address */
	const node_ref *elem_end = node->elems + node->hdr.cnt;
	for (node_ref *elem = node->elems; elem < elem_end; ++elem) {
		if (elem->addr == addr) {
			return elem;
		}
	}
	
	/* not found */
	return NULL;
}

void branch_zero(branch_ptr node, uint16_t first) {
	ASSERT_BRANCH(node);
	
	uint8_t *zero_begin = (uint8_t *)(node->elems + first);
	uint8_t *zero_end   = (uint8_t *)(node->elems + node->hdr.cnt);
	
	memset(zero_begin, 0, (zero_end - zero_begin));
}

void branch_shift_forward(branch_ptr node, uint16_t first, uint16_t diff) {
	ASSERT_BRANCH(node);
	if (first >= node->hdr.cnt) {
		return;
	}
	
	node_ref *elem_first = node->elems + first;
	node_ref *elem_last  = node->elems + (node->hdr.cnt - 1);
	
	for (node_ref *elem = elem_last; elem >= elem_first; --elem) {
		node_ref *elem_dst = elem + diff;
		
		*elem_dst = *elem;
	}
}

void branch_shift_backward(branch_ptr node, uint16_t first, uint16_t diff) {
	ASSERT_BRANCH(node);
	if (first >= node->hdr.cnt) {
		return;
	}
	
	node_ref *elem_first = node->elems + first;
	node_ref *elem_last  = node->elems + (node->hdr.cnt - 1);
	
	for (node_ref *elem = elem_first; elem <= elem_last; ++elem) {
		node_ref *elem_dst = elem - diff;
		
		*elem_dst = *elem;
	}
}

void branch_xfer(branch_ptr dst, const branch_ptr src, uint16_t dst_idx,
	uint16_t src_idx, uint16_t cnt) {
	ASSERT_BRANCH(dst);
	ASSERT_BRANCH(src);
	
	for (uint16_t i = 0; i < cnt; ++i) {
		branch_insert_naive(dst, dst_idx + i, src->elems + (src_idx + i));
	}
}

void branch_append_naive(branch_ptr node, const node_ref *elem) {
	ASSERT_BRANCH(node);
	
	node->elems[node->hdr.cnt++] = *elem;
}

void branch_insert_naive(branch_ptr node, uint16_t at, const node_ref *elem) {
	ASSERT_BRANCH(node);
	
	*(node->elems + at) = *elem;
}

bool branch_insert(branch_ptr node, const node_ref *elem) {
	ASSERT_BRANCH(node);
	
	/* the caller needs to make space if necessary */
	if (branch_free(node) < sizeof(node_ref)) {
		return false;
	}
	
	uint16_t insert_at = node_search_hypo((node_ptr)node, &elem->key);
	
	branch_shift_forward(node, insert_at, 1);
	branch_insert_naive(node, insert_at, elem);
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

bool branch_remove(branch_ptr node, const key *key) {
	ASSERT_BRANCH(node);
	
	
}

void branch_paternalize(branch_ptr node) {
	ASSERT_BRANCH(node);
	
	const node_ref *elem_end = node->elems + node->hdr.cnt;
	for (const node_ref *elem = node->elems; elem < elem_end; ++elem) {
		node_ptr child = node_map(elem->addr, true);
		child->hdr.parent = node->hdr.this;
		node_unmap(child);
	}
}

void branch_ref(branch_ptr node, node_ptr child) {
	ASSERT_BRANCH(node);
	
	if (branch_free(node) < sizeof(node_ref)) {
		errx("%s: no space: node 0x%" PRIx32 " child 0x%" PRIx32,
			__func__, node->hdr.this, child->hdr.this);
	}
	
	node_ref elem;
	elem.key = *node_first_key(child);
	elem.addr = child->hdr.this;
	
	branch_insert(node, &elem);
}

void branch_ref_update(branch_ptr node, node_ptr child) {
	ASSERT_BRANCH(node);
	
	node_ref *elem = branch_search_addr(node, child->hdr.this);
	if (elem != NULL) {
		elem->key = *node_first_key(child);
		
		/* if we are not root and we just updated the element in position 0, we
		 * need to update the node_ref to us in our parent */
		if (elem == node->elems && node->hdr.parent != 0) {
			branch_ptr parent = (branch_ptr)node_map(node->hdr.parent, true);
			branch_ref_update(parent, (node_ptr)node);
			node_unmap((node_ptr)parent);
		}
	} else {
		errx("%s: not found: node 0x%" PRIx32 " child 0x%" PRIx32,
			__func__, node->hdr.this, child->hdr.this);
	}
}

void branch_split_post(branch_ptr this, branch_ptr new, bool was_root) {
	ASSERT_BRANCH(this);
	ASSERT_BRANCH(new);
	
	uint16_t half = branch_half(this);
	
	if (half == 0) {
		errx("%s: half == 0", __func__);
	} else if (half == this->hdr.cnt) {
		errx("%s: half == this->hdr.cnt", __func__);
	}
	
	branch_xfer(new, this, 0, half, this->hdr.cnt - half);
	new->hdr.cnt = this->hdr.cnt - half;
	
	branch_zero(this, half);
	this->hdr.cnt = half;
	
	/* our children don't know who their parent is anymore */
	branch_paternalize(new);
	if (was_root) {
		branch_paternalize(this);
	}
}
