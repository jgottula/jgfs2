#include "key.h"
#include "../debug.h"


int8_t key_cmp(const key *lhs, const key *rhs) {
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

const char *key_str(const key *key) {
	static char buf[1024];
	
	snprintf(buf, sizeof(buf),
		"[ id %" PRIu32 " type %" PRIu8 " off %" PRIu32 "]",
		key->id, key->type, key->off);
	
	return buf;
}
