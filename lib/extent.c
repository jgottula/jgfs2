#include "extent.h"
#include "debug.h"


/* NOTE: allocations CANNOT result in insertions to the ext tree because tree
 * insertions themselves rely on successful allocation */
uint32_t ext_alloc(uint32_t len) {
	/* traverse the ext tree horizontally and find a free extent that is
	 * large enough */
	
	static uint32_t alloc_ptr = 100;
	
	TODO("remove dummy code");
	return alloc_ptr++;
}

/* TODO: look into lazy allocation features of linux for when we have to get
 * zeroed extents (holes) for the library user (or will we just use the
 * read(2)/write(2) convention of filling a user buffer?) */
