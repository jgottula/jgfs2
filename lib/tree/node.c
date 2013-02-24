#include "node.h"


void node_dump(uint32_t node_addr) {
	struct node_hdr *hdr = fs_map_blk(node_addr, node_size_blk());
	
	if (hdr->leaf) {
		leaf_dump((const leaf_ptr)hdr);
	} else {
		branch_dump((const branch_ptr)hdr);
	}
	
	fs_unmap_blk(hdr, node_addr, node_size_blk());
}

node_ptr node_map(uint32_t node_addr) {
	return (node_ptr)fs_map_blk(node_addr, node_size_blk());
}

void node_unmap(const node_ptr node) {
	const struct node_hdr *hdr = (const struct node_hdr *)node;
	
	fs_unmap_blk(node, hdr->this, node_size_blk());
}
