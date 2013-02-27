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
	node_dump(fs.sblk->s_addr_meta_tree);
}

static void test_check(void) {
	struct check_result result = check_node(fs.sblk->s_addr_meta_tree);
	check_print(result, true);
}

static bool test_check_nonfatal(void) {
	struct check_result result = check_node(fs.sblk->s_addr_meta_tree);
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
	
	struct inode_item inode;
	memset(&inode, 0x88, sizeof(inode));
	key key = {
		0x88888888,
		KEY_INODE,
		0,
	};
	/*tree_insert(fs.sblk->s_addr_meta_tree, &key,
		(struct item_data){ sizeof(inode), &inode });
	test_check();
	
	memset(&inode, 0xcc, sizeof(inode));
	key.id = 0xcccccccc;
	tree_insert(fs.sblk->s_addr_meta_tree, &key,
		(struct item_data){ sizeof(inode), &inode });
	test_check();
	
	memset(&inode, 0x44, sizeof(inode));
	key.id = 0x44444444;
	tree_insert(fs.sblk->s_addr_meta_tree, &key,
		(struct item_data){ sizeof(inode), &inode });
	test_check();
	
	memset(&inode, 0x55, sizeof(inode));
	key.id = 0x55555555;
	tree_insert(fs.sblk->s_addr_meta_tree, &key,
		(struct item_data){ sizeof(inode), &inode });
	test_check();*/
	
	for (uint32_t i = 0x0; i < 0x10; ++i) {
		key.id = i * 0x10000000;
		tree_insert(fs.sblk->s_addr_meta_tree, &key,
			(struct item_data){ sizeof(inode), &inode });
		test_dump();
	}
	
	//key.id = 0x00000000;
	for (uint32_t i = 0; i < 70; ++i) {
		memset(&inode, lrand48(), sizeof(inode));
		key.id = lrand48();
		
		fprintf(stderr, "#%" PRId32 ": %" PRIx32 "\n", i, key.id);
		
		tree_insert(fs.sblk->s_addr_meta_tree, &key,
			(struct item_data){ sizeof(inode), &inode });
		
		if (!test_check_nonfatal()) {
			fprintf(stderr, "%s: failure on #%" PRId32 "\n", __func__, i);
			test_dump();
			
			abort();
		}
	}
	
	test_dump();
	
	jgfs2_done();
}
