/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "insert.h"
#include <err.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../lib/fs.h"
#include "../../../lib/tree.h"
#include "../argp.h"
#include "../help.h"
#include "../rand.h"


uint32_t meta;


bool test_insert(uint32_t cnt) {
	srand48(param.rand_seed);
	
	help_new();
	meta = fs.sblk->s_addr_meta_tree;
	FAIL_ON(help_check_tree(meta));
	
	uint8_t data[4096];
	rand32_fill_range((uint32_t *)data, sizeof(data) / sizeof(uint32_t),
		UINT32_MAX);
	
	key the_key = {
		0x00000000,
		0x00,
		0x00000000,
	};
	
	cnt = (1 << cnt);
	
	warnx("permuting key_ids array");
	uint32_t *key_ids = malloc(sizeof(uint32_t) * cnt);
	rand32_permute_init(key_ids, cnt);
	
	warnx("generating item_lens array");
	uint32_t *item_lens = malloc(sizeof(uint32_t) * cnt);
	rand32_fill_range(item_lens, cnt, 400);
	
	fprintf(stderr, "total %" PRIu32 "\n", cnt);
	
	for (uint32_t i = 0; i < cnt; ++i) {
		if ((i + 1) % 100 == 0 || i == cnt - 1) {
			fprintf(stderr, "\rinsert %" PRIu32, i + 1);
		}
		
		the_key.id = key_ids[i];
		uint32_t len = item_lens[the_key.id];
		
		tree_insert(meta, &the_key, (struct item_data){ len, data });
	}
	fputc('\n', stderr);
	
	warnx("tree check");
	help_check_tree(meta);
	
	the_key.id = 0;
	leaf_ptr node = tree_search(meta, &the_key);
	for (uint32_t i = 0; i < cnt; ++i) {
		if ((i + 1) % 100 == 0 || i == cnt - 1) {
			fprintf(stderr, "\rcheck %" PRIu32, i + 1);
		}
		
		the_key.id = i;
		uint32_t len = item_lens[i];
		
		uint16_t item_idx;
		
		/* FAST node elem search using leaf node linked list */
		while (!node_search((node_ptr)node, &the_key, &item_idx)) {
			if (node->hdr.next == 0) {
				warnx("node_search fail at i = %" PRIu32 "\n", i);
				abort();
			}
			
			leaf_ptr next = (leaf_ptr)node_map(node->hdr.next);
			node_unmap((node_ptr)node);
			node = next;
		}
		
		item_ref *item = node->elems + item_idx;
		
		if (item->len != len) {
			warnx("wrong len: i = %" PRIu32 " item->len = %" PRIu32 " len = %"
				PRIu32 "\n", i, item->len, len);
			abort();
		}
		
		uint8_t *item_data = leaf_data_ptr(node, item_idx);
		if (memcmp(item_data, data, item->len) != 0) {
			warnx("bad data: i = %" PRIu32 "\n", i);
			abort();
		}
	}
	node_unmap((node_ptr)node);
	fputc('\n', stderr);
	
	//tree_graph(meta);
	jgfs2_done();
	return true;
}
