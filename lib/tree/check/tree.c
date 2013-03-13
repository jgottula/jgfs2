/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../check.h"
#include "../../debug.h"


struct check_result check_tree(uint32_t root_addr) {
	struct check_result result = { RESULT_TYPE_OK };
	
	TODO("tree-specific checks");
	// check for overall tree consistency:
	// dupes between nodes
	// node n's largest key not less than node n+1's smallest
	
	result = check_node(root_addr, true);
	if (result.type != RESULT_TYPE_OK) {
		goto done;
	}
	
done:
	return result;
}
