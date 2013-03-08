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

void test_tree0(void) {
	fprintf(stderr, "%s\n", __func__);
	
	jgfs2_new("/dev/loop0p1",
		&(struct jgfs2_mount_options){
			.read_only = false,
			.debug_map = false,
		},
		&(struct jgfs2_mkfs_param){
			.boot_sect = 14,
			.zap_vbr = true,
			.zap_boot = true,
		}
	);
	test_check();
	
	key key = {
		0x00000000,
		0x00,
		0x00000000,
	};
	uint8_t data[4096];
	
	for (size_t i = 0; i < sizeof(data); ++i) {
		data[i] = (uint8_t)i;
	}
	
	for (uint32_t i = 0; i < 10000; ++i) {
		key.id = ((i % 1000) * 10) + (i / 1000);
		uint32_t len = i % 200;
		
		tree_insert(fs.sblk->s_addr_meta_tree, &key,
			(struct item_data){ len, data });
	}
	
	for (uint32_t i = 0; i < 10000; ++i) {
		key.id = ((i % 1000) * 10) + (i / 1000);
		uint32_t len = i % 200;
		
		leaf_ptr leaf = tree_search(fs.sblk->s_addr_meta_tree, &key);
		uint16_t item_idx;
		
		if (!node_search((node_ptr)leaf, &key, &item_idx)) {
			fprintf(stderr, "node_search fail at i = %" PRIu32 "\n", i);
			abort();
		}
		
		item_ref *item = leaf->elems + item_idx;
		
		if (item->len != len) {
			fprintf(stderr, "len wrong: i = %" PRIu32 " is %" PRIu32
				" should be %" PRIu32 "\n", i, item->len, len);
			abort();
		}
		
		uint8_t *item_data = leaf_data_ptr(leaf, item_idx);
		
		for (uint32_t j = 0; j < len; ++j) {
			if (item_data[j] != (uint8_t)j) {
				fprintf(stderr, "bad data: i = %" PRIu32 " j = %" PRIu32
					" item_data[j] = %" PRIu8 "\n", i, j, item_data[j]);
				abort();
			}
		}
		
		node_unmap((node_ptr)leaf);
	}
	
	test_check();
	//tree_dump(fs.sblk->s_addr_meta_tree);
	tree_graph(fs.sblk->s_addr_meta_tree);
	
	jgfs2_done();
}
