/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "extent.h"
#include "debug.h"


/* TODO: need a higher-level function that will return a number of extents that
 * add up to the requested length if a single extent can't be found;
 * we can use the fact that we have access to the ext tree to allocate the
 * largest extents that we know about so files have the fewest possible frags;
 * use this when allocating file data extents */

uint32_t ext_alloc(uint32_t len) {
	/* traverse the ext tree horizontally and find a free extent that is
	 * large enough */
	
	static uint32_t alloc_ptr = 100;
	
	//TODO("remove dummy code");
	return alloc_ptr++;
}

/* TODO: look into lazy allocation features of linux for when we have to get
 * zeroed extents (holes) for the library user (or will we just use the
 * read(2)/write(2) convention of filling a user buffer?) */

/* FIX for the tree->aloc->tree feedback loop:
 * 
 * add new param to ext_alloc, ext_dealloc called 'in_mem', used by node_split,
 * that is set when node_split is operating on the ext tree
 * 
 * when in_mem, keep track of alloc'd and dealloc'd regions in a list/tree
 * when in_mem, consult list/tree in addition to the ext tree itself
 * PROBLEM: what if the ext tree is in a state of temporary brokenness?
 * 
 * when NOT in_mem, complain exceedingly loudly if the list/tree is not empty
 * 
 * new function, ext_flush, that "flushes" the list by calling ext_(de)alloc
 * (note that those functions may, in turn, alter the list if they have issues)
 * 
 * update node_split to use in_mem
 * update node_split to call ext_flush at the END of its TOP-LEVEL recursion
 */

/* IDEA: two ext trees that track only free space
 * 
 * one is sorted by offset
 * the other is sorted by length
 * 
 * use the one sorted by length for allocations (find just the right size)
 * use the one sorted by offset for deallocations (find the addr we need)
 * 
 * when one tree is modified, update the other
 */
