#include "node.h"
#include <inttypes.h>
#include "debug.h"
#include "fs.h"


bool node_item(uint32_t node_addr, const struct jgfs2_key *key,
	void **found_data) {
	struct jgfs2_node *node = fs_map_blk(node_addr, 1);
	bool status = false;
	
	if (!node->hdr.leaf) {
		errx(1, "%s: not a leaf node: %" PRIu32, __func__, node_addr);
	}
	
	for (uint16_t i = 0; i < node->hdr.item_qty; ++i) {
		const struct jgfs2_item *item = &node->items[i];
		
		if (key_cmp(key, &item->key) == 0) {
			*found_data = (uint8_t *)node + item->off;
		}
	}
	
	fs_unmap_blk(node, node_addr, 1);
	
	return status;
}

uint32_t node_space(uint32_t node_addr) {
	struct jgfs2_node *node = fs_map_blk(node_addr, 1);
	uint32_t space = 0;
	
	if (!node->hdr.leaf) {
		errx(1, "%s: not a leaf node: %" PRIu32, __func__, node_addr);
	}
	
	TODO("test this");
	
	intptr_t space_end =
		(intptr_t)node + node->items[node->hdr.item_qty - 1].off;
	intptr_t space_begin =
		(intptr_t)(&node->items[node->hdr.item_qty]);
	
	space = space_end - space_begin;
	
	fs_unmap_blk(node, node_addr, 1);
	
	return space;
}

uint16_t node_split_point(uint32_t node_addr) {
	struct jgfs2_node *node = fs_map_blk(node_addr, 1);
	
	if (!node->hdr.leaf) {
		errx(1, "%s: not a leaf node: %" PRIu32, __func__, node_addr);
	}
	
	TODO("test this");
	
	uint32_t used_items =
		node->hdr.item_qty * sizeof(struct jgfs2_item);
	uint32_t used_data_begin =
		node->items[node->hdr.item_qty - 1].off;
	
	uint32_t used = used_items + (fs.blk_size - used_data_begin);
	
	uint16_t split_at = node->hdr.item_qty / 2;
	uint32_t used_incr = 0;
	for (uint16_t i = node->hdr.item_qty; i-- != 0; ) {
		used_incr += sizeof(struct jgfs2_item);
		used_incr += node->items[i].len;
		
		if (used_incr <= used / 2) {
			split_at = i;
			break;
		}
	}
	
	fs_unmap_blk(node, node_addr, 1);
	
	return split_at;
}

void node_dump(uint32_t node_addr) {
	struct jgfs2_node *node = fs_map_blk(node_addr, 1);
	
	warnx("%s: %s node %" PRIu32 " with %" PRIu16 " %ss", __func__,
		(node->hdr.leaf ? "leaf" : "non-leaf"), node_addr, node->hdr.item_qty,
		(node->hdr.leaf ? "item" : "node ptr"));
	
	for (uint16_t i = 0; i < node->hdr.item_qty; ++i) {
		if (node->hdr.leaf) {
			const struct jgfs2_item *item = &node->items[i];
			
			fprintf(stderr, "item %3" PRIu16 ": id %" PRIu32 " type %" PRIu8 
				" off %" PRIu32 "\n",
				i, item->key.id, item->key.type, item->key.off);
		} else {
			const struct jgfs2_node_ptr *child = &node->children[i];
			
			fprintf(stderr, "child %3" PRIu16 ": id %" PRIu32 " type %" PRIu8 
				" off %" PRIu32 "\naddr %08" PRIx32,
				i, child->key.id, child->key.type, child->key.off, child->addr);
		}
	}
	
	uint8_t *node_bytes = (uint8_t *)node;
	for (size_t i = 0; i < fs.blk_size; ++i) {
		if (i % 16 == 0) {
			fprintf(stderr, "%04zx: ", i);
		} else if (i % 16 == 8) {
			fputc(' ', stderr);
		}
		
		fprintf(stderr, " %02x", node_bytes[i]);
		
		if (i % 16 == 15) {
			fputc('\n', stderr);
		}
	}
	
	fs_unmap_blk(node, node_addr, 1);
}

void node_xfer(struct jgfs2_node *dst, const struct jgfs2_node *src,
	uint16_t src_idx) {
	uint16_t dst_idx = dst->hdr.item_qty;
	const struct jgfs2_item *src_item = &src->items[src_idx];
	struct jgfs2_item *dst_item = &dst->items[dst_idx];
	
	if (!src->hdr.leaf) {
		errx(1, "%s: not a leaf node: src", __func__);
	} else if (!dst->hdr.leaf) {
		errx(1, "%s: not a leaf node: dst", __func__);
	}
	
	*dst_item = *src_item;
	
	if (dst_idx > 0) {
		dst_item->off = dst->items[dst_idx - 1].off - src_item->len;
	} else {
		dst_item->off = fs.blk_size - src_item->len;
	}
	
	memcpy((uint8_t *)dst + dst_item->off, (uint8_t *)src + src_item->off,
		src_item->len);
	
	++dst->hdr.item_qty;
}
