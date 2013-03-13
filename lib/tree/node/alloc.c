/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../node.h"
#include "../../debug.h"
#include "../../extent.h"


uint32_t node_alloc(void) {
	return ext_alloc(node_size_blk());
}

void node_dealloc(uint32_t node_addr) {
	TODO("implement this");
	/*ext_dealloc(node_addr, node_size_blk());*/
}
