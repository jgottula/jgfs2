#ifndef JGFS2_LIB_TREE_KEY_H
#define JGFS2_LIB_TREE_KEY_H


#include "../jgfs2.h"


typedef struct __attribute__((__packed__)) {
	uint32_t id;
	uint8_t  type;
	uint32_t off;
} key;


int8_t key_cmp(const key *lhs, const key *rhs);
const char *key_str(const key *key);


#endif
