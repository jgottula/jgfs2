#ifndef JGFS2_LIB_TREE_H
#define JGFS2_LIB_TREE_H


#include "jgfs2.h"


struct __attribute__((__packed__)) jgfs2_key {
	uint32_t id;
	uint8_t  type;
	uint32_t off;
};

struct __attribute__((__packed__)) jgfs2_item {
	struct jgfs2_key key;
	uint32_t off;
	uint32_t len;
};

struct __attribute__((__packed__)) jgfs2_node_ptr {
	struct jgfs2_key key;
	uint32_t addr;
};

struct __attribute__((__packed__)) jgfs2_node_hdr {
	bool leaf;
	uint32_t parent_addr;
	uint32_t next_addr;
	uint16_t item_qty;
};

struct __attribute__((__packed__)) jgfs2_node {
	struct jgfs2_node_hdr hdr;
	
	union {
		struct jgfs2_item items[0];
		struct jgfs2_node_ptr children[0];
	};
};


int8_t jgfs2_key_cmp(const struct jgfs2_key *lhs, const struct jgfs2_key *rhs);

bool jgfs2_node_item(uint32_t node_addr, const struct jgfs2_key *key,
	void **found_data);
uint32_t jgfs2_node_space(uint32_t node_addr);
void jgfs2_node_dump(uint32_t node_addr);

void jgfs2_tree_init(uint32_t root_addr);
void jgfs2_tree_split(uint32_t node_addr);
uint32_t jgfs2_tree_search(uint32_t node_addr, const struct jgfs2_key *key);

#endif
