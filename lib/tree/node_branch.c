#include "node.h"
#include "../debug.h"


void branch_dump(const branch_ptr node) {
	const struct node_hdr *hdr = &node->hdr;
	
	warnx("%s: this 0x%08" PRIx32 " parent 0x%08" PRIx32 " cnt %" PRIu16,
		__func__, hdr->this, hdr->parent, hdr->cnt);
	
	const node_ref *elem_end = node->elems + node->hdr.cnt;
	for (const node_ref *elem = node->elems; elem < elem_end; ++elem) {
		fprintf(stderr, "node %3" PRIdPTR ": %s addr 0x%08" PRIx32 "\n",
			(elem - node->elems), key_str(&elem->key), elem->addr);
	}
	
	dump_mem(node, sizeof(*node));
}

branch_ptr branch_init(uint32_t node_addr, uint32_t parent) {
	branch_ptr node = (branch_ptr)node_map(node_addr);
	
	node->hdr.leaf   = false;
	node->hdr.cnt    = 0;
	node->hdr.this   = node_addr;
	node->hdr.next   = 0;
	node->hdr.parent = parent;
	
	return node;
}

uint32_t branch_used(const branch_ptr node) {
	return node->hdr.cnt * sizeof(node_ref);
}

uint32_t branch_free(const branch_ptr node) {
	return node_size_byte() - (sizeof(struct node_hdr) + branch_used(node));
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

void branch_xfer_half(branch_ptr dst, branch_ptr src) {
	uint16_t half = branch_half(src);
	
	if (half == 0) {
		errx(1, "%s: half == 0", __func__);
	} else if (half == src->hdr.cnt) {
		errx(1, "%s: half == src->hdr.cnt", __func__);
	}
	
	const node_ref *elem_end = src->elems + src->hdr.cnt;
	for (const node_ref *elem = src->elems + half; elem < elem_end; ++elem) {
		branch_append_naive(dst, elem);
	}
	
	branch_zero(src, half);
}

void branch_assert_parenthood(branch_ptr node) {
	const node_ref *elem_end = node->elems + node->hdr.cnt;
	for (const node_ref *elem = node->elems; elem < elem_end; ++elem) {
		node_ptr child = node_map(elem->addr);
		child->hdr.parent = node->hdr.this;
		node_unmap(child);
	}
}

void branch_append_naive(branch_ptr node, const node_ref *elem) {
	node->elems[node->hdr.cnt++] = *elem;
}

bool branch_insert(branch_ptr node, const node_ref *elem) {
	/* the caller needs to make space if necessary */
	if (branch_free(node) < sizeof(node_ref)) {
		return false;
	}
	
	if (node->hdr.cnt == 0) {
		errx(1, "%s: cannot be empty: node %08" PRIx32,
			__func__, node->hdr.this);
	}
	
	/* insert at the end if highest key */
	uint16_t insert_at;
	const node_ref *elem_last = node->elems + (node->hdr.cnt - 1);
	if (key_cmp(&elem->key, &elem_last->key) > 0) {
		insert_at = node->hdr.cnt;
	} else {
		uint16_t i = node->hdr.cnt;
		while (i-- != 0) {
			if (key_cmp(&elem->key, &node->elems[i].key) < 0) {
				insert_at = i;
				break;
			}
			
			node->elems[i + 1] = node->elems[i];
		}
	}
	
	node->elems[insert_at] = *elem;
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

void branch_ref(branch_ptr node, node_ptr child) {
	if (branch_free(node) < sizeof(node_ref)) {
		errx(1, "%s: no space: node %08" PRIx32 " child %08" PRIx32,
			__func__, node->hdr.this, child->hdr.this);
	}
	
	if (child->hdr.cnt == 0) {
		errx(1, "%s: empty child: node %08" PRIx32 " child %08" PRIx32,
			__func__, node->hdr.this, child->hdr.this);
	}
	
	node_ref elem;
	if (child->hdr.leaf) {
		elem.key = ((leaf_ptr)child)->elems[0].key;
	} else {
		elem.key = ((branch_ptr)child)->elems[0].key;
	}
	elem.addr = child->hdr.this;
	
	branch_insert(node, &elem);
}

void branch_ref_update(branch_ptr node, node_ptr child) {
	if (child->hdr.cnt == 0) {
		errx(1, "%s: empty child: node %08" PRIx32 " child %08" PRIx32,
			__func__, node->hdr.this, child->hdr.this);
	}
	
	bool found = false;
	
	const node_ref *elem_end = node->elems + node->hdr.cnt;
	for (node_ref *elem = node->elems; elem < elem_end; ++elem) {
		if (elem->addr == child->hdr.this) {
			if (child->hdr.leaf) {
				elem->key = ((leaf_ptr)child)->elems[0].key;
			} else {
				elem->key = ((branch_ptr)child)->elems[0].key;
			}
			
			found = true;
			break;
		}
	}
	
	if (!found) {
		errx(1, "%s: not found: node %08" PRIx32 " child %08" PRIx32,
			__func__, node->hdr.this, child->hdr.this);
	}
}

void branch_split_post(branch_ptr this, branch_ptr new, bool was_root) {
	branch_xfer_half(new, this);
	
	/* our children don't know who their parent is anymore */
	branch_assert_parenthood(new);
	if (was_root) {
		branch_assert_parenthood(this);
	}
}