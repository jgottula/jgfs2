#ifndef JGFS2_LIB_NODE_H
#define JGFS2_LIB_NODE_H


#include "jgfs2.h"
#include "tree.h"


bool node_item(uint32_t node_addr, const struct jgfs2_key *key,
	void **found_data);
uint32_t node_space(uint32_t node_addr);
uint16_t node_split_point(uint32_t node_addr);
void node_dump(uint32_t node_addr);
void node_xfer(struct jgfs2_node *dst, const struct jgfs2_node *src,
	uint16_t src_idx);


#endif
