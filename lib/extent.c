#include "extent.h"
#include "debug.h"


/* NOTE: allocations CANNOT result in insertions to the ext tree because tree
 * insertions themselves rely on successful allocation */
uint32_t ext_alloc(uint32_t len) {
	/* traverse the ext tree horizontally and find a free extent that is
	 * large enough */
	
	TODO("remove dummy code");
	return 100;
}


