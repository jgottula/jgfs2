/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


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


/* POSSIBLE fix for tree->alloc->tree feedback loop:
 * FIRST, look at btrfs and figure out if file extents are merged in the ext
 * allocation tree and if free extents are kept as items or what so we know what
 * we're dealing with
 * 
 * the stuff listed below won't quite work because the tree will be locked for
 * read access too; so, it would be best to cache the largest free extents in
 * a linked list in memory and make temporary allocations from those, and then
 * commit the modifications to memory when the ext tree is unlocked
 *
 * THIS IS NOT QUITE RIGHT YET:
 * - implement tree locking
 * - keep a linked list of temporary modifications
 * - if the ext tree is locked when we try to allocate, commit modifications to
 *   the linked list instead and consult the linked list
 * - if the ext tree is not locked and the linked list hasn't been flushed,
 *   explode in the user's face
 * - add a function that node_split can call when its original (first) recursion
 *   finishes that will flush the linked list to the ext tree
 */
