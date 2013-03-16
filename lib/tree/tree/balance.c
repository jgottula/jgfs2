/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../tree.h"
#include "../../debug.h"


/* modify parameters and then run benchmarks on (1) avg node utilization,
 * (2) percentage of insertions/deletions that resulted in balance operations,
 * and (3) avg length of time for a balance operation */


// for branch nodes (fixed key weight):
//  B = 0
// 2Q = node_space_usable() / sizeof(node_ref)
//  P = node_space_usable()


// for leaf nodes (variable key weight):
/*#define B 2          // locality parameter: nodes per sweep
#define Q ???        // order: min keys per node
#define P 4077       // rank: max node weight
#define MU_MAX_K 256 // max key weight


#define NEIGHBORS  B
#define SWEEP_LEN  (B + 1)
#define CANDIDATES ((B * 2) - 1)*/


node_ptr tree_split_single(node_ptr node, const key *key,
	uint32_t space_needed) {
	
	// split in 1:2 fashion (normal btree split)
	// split to the right always
	// make space for the insertion key when splitting
	// insert the key in that spot
	
	// handle root, of course
	
	// need to update parent refs (before or after?)
	// (ensure that ptrs/addrs don't change unexpectedly)
	
	// return pointer to node with spot for the key
	// caller needs to fill the elem data afterward (if appropriate)
	
	return NULL;
}

node_ptr tree_split_pair(node_ptr nodes[2], const key *key,
	uint32_t space_needed) {
	
	// similar to tree_split_single
	
	// can put the new node to the left, in the middle, or to the right
	// optimally, we choose whichever direction results in the least transfers
	
	
	
	
	return NULL;
}

void tree_merge_single(node_ptr nodes[2], const key *key) {
	// merge 2->1
	// at the same time, remove the key given
	
	// caller needs to pre-zero elem data beforehand if necessary
}

void tree_merge_pair(node_ptr nodes[3], const key *key) {
	
}
