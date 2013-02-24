#include "key.h"


int8_t key_cmp(const struct jgfs2_key *lhs, const struct jgfs2_key *rhs) {
	if (lhs->id > rhs->id) {
		return 1;
	} else if (lhs->id < rhs->id) {
		return -1;
	}
	
	if (lhs->type > rhs->type) {
		return 1;
	} else if (lhs->type < rhs->type) {
		return -1;
	}
	
	if (lhs->off > rhs->off) {
		return 1;
	} else if (lhs->off < rhs->off) {
		return -1;
	}
	
	return 0;
}
