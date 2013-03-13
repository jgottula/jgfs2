/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../check.h"
#include "../../debug.h"


struct check_result check_item(const key *key, struct item_data item) {
	struct check_result result = { RESULT_TYPE_OK };
	
	// test here
	// when any test fails, set result and goto done
	
	// check: type validity
	// check: key.id appropriateness
	// check: key.off appropriateness
	// check: size appropriateness
	// check: contents of items by type
	
//done:
	return result;
}
