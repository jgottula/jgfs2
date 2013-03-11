/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include <time.h>
#include "rand.h"
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

void test_tree1(void) {
	fprintf(stderr, "%s\n", __func__);
	
	jgfs2_new("/dev/loop0",
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
	
#define MAX (1 << 20)
	
	srand(time(NULL));
	long seed = rand();
	
	fprintf(stderr, "seed: srand48(%ld)\n", seed);
	srand48(seed);
	
	fprintf(stderr, "permuting key id array\n");
	uint32_t *keys = malloc(sizeof(uint32_t) * MAX);
	rand32_permute_init(keys, MAX);
	
	fprintf(stderr, "generating data len array\n");
	uint32_t *lens = malloc(sizeof(uint32_t) * MAX);
	rand32_fill_range(lens, MAX, 400);
	
	fprintf(stderr, "total %" PRIu32 "\n", MAX);
	
	for (uint32_t i = 0; i < MAX; ++i) {
		if (i % 100 == 0 || i == MAX - 1) {
			fprintf(stderr, "\rinsert %" PRIu32, i + 1);
		}
		
		key.id = keys[i];
		uint32_t len = lens[i];
		
		tree_insert(fs.sblk->s_addr_meta_tree, &key,
			(struct item_data){ len, data });
		//test_check();
	}
	fputc('\n', stderr);
	
	test_check();
	
	key.id = 0;
	leaf_ptr node = tree_search(fs.sblk->s_addr_meta_tree, &key);
	for (uint32_t i = 0; i < MAX; ++i) {
		if (i % 100 == 0 || i == MAX - 1) {
			fprintf(stderr, "\rcheck %" PRIu32, i + 1);
		}
		
		key.id = i;
		uint32_t len = lens[i];
		
		uint16_t item_idx;
		
		/* FAST node elem search using leaf node linked list */
		while (!node_search((node_ptr)node, &key, &item_idx)) {
			if (node->hdr.next == 0) {
				fprintf(stderr, "node_search fail at i = %" PRIu32 "\n", i);
				abort();
			}
			
			leaf_ptr next = (leaf_ptr)node_map(node->hdr.next);
			node_unmap((node_ptr)node);
			node = next;
		}
		
		item_ref *item = node->elems + item_idx;
		
		if (item->len != len) {
			fprintf(stderr, "wrong len: i = %" PRIu32 " item->len = %" PRIu32
				" len = %" PRIu32 "\n", i, item->len, len);
			abort();
		}
		
		uint8_t *item_data = leaf_data_ptr(node, item_idx);
		
		for (uint32_t j = 0; j < item->len; ++j) {
			if (item_data[j] != (uint8_t)j) {
				fprintf(stderr, "bad data: i = %" PRIu32 " j = %" PRIu32
					" item_data[j] = %" PRIu8 "\n", i, j, item_data[j]);
				abort();
			}
		}
	}
	node_unmap((node_ptr)node);
	fputc('\n', stderr);
	
#if 0
	for (uint32_t i = 0; i < MAX; ++i) {
		if (i % 100 == 0 || i == MAX - 1) {
			fprintf(stderr, "\rremove %" PRIu32, i + 1);
		}
		
		key.id = keys[i];
		
		if (!tree_remove(fs.sblk->s_addr_meta_tree, &key)) {
			fprintf(stderr, "tree_remove fail at i = %" PRIu32 "\n", i);
			abort();
		}
		
		test_check();
	}
	fputc('\n', stderr);
#endif
	
	tree_graph(fs.sblk->s_addr_meta_tree);
	
	jgfs2_done();
}
