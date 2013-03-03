/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../../lib/jgfs2.h"
#include "../../lib/debug.h"
#include "../../lib/fs.h"
#include "../../lib/tree.h"


static void test_dump(void) {
	tree_graph(fs.sblk->s_addr_meta_tree);
	tree_dump(fs.sblk->s_addr_meta_tree);
}

static void test_check(void) {
	struct check_result result = check_tree(fs.sblk->s_addr_meta_tree);
	check_print(result, true);
}

static bool test_check_nonfatal(void) {
	struct check_result result = check_tree(fs.sblk->s_addr_meta_tree);
	check_print(result, false);
	return (result.type == RESULT_TYPE_OK);
}

void test_tree(void) {
	fprintf(stderr, "%s\n", __func__);
	
	jgfs2_init("/dev/loop0p1",
		&(struct jgfs2_mount_options){
			.read_only = false,
			.debug_map = false,
		});
	test_check();
	
	fprintf(stderr, "%s: inserting inode\n", __func__);
	
	key key = {
		0x00000000,
		0x00,
		0x00000000,
	};
	
	for (uint32_t i = 0; i < 300; ++i) {
		key.id = mrand48() & 0xffff;
		
		fprintf(stderr, "#%" PRId32 ": %" PRIx32 "\n", i, key.id);
		
		tree_insert(fs.sblk->s_addr_meta_tree, &key,
			(struct item_data){ 0, NULL });
		test_dump();
		
		if (!test_check_nonfatal()) {
			fprintf(stderr, "%s: failure on #%" PRId32 "\n", __func__, i);
			abort();
		}
	}
	
	test_dump();
	
	jgfs2_done();
}
