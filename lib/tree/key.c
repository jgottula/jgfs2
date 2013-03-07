/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


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
	static char buf[23];
	
	snprintf(buf, sizeof(buf), "[%08" PRIx32 "|%02" PRIx8 "|%08" PRIx32 "]",
		key->id, key->type, key->off);
	
	return buf;
}
