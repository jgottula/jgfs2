/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "tree.h"
#include <math.h>
#include "../debug.h"
#include "check.h"


void tree_init(uint32_t root_addr) {
	node_unmap((node_ptr)leaf_init(root_addr, 0, 0, 0));
}

void tree_dump(uint32_t root_addr) {
	ASSERT_ROOT(root_addr);
	node_dump(root_addr, true);
}

static void tree_graph_r(uint32_t node_addr, uint32_t level, uint32_t *max) {
	node_ptr node = node_map(node_addr);
	
	/* draw the node space usage bar */
	uint8_t used_round = (uint8_t)ceil(((float)node_used(node) * 12.f) /
		(float)node_size_usable());
	char used_bar[13];
	for (uint8_t i = 0; i < 12; ++i) {
		if (i <= used_round - 1) {
			used_bar[i] = '#';
		} else {
			used_bar[i] = ' ';
		}
	}
	used_bar[sizeof(used_bar) - 1] = '\0';
	
	float used_pct = ((float)node_used(node) * 100.f) /
		(float)node_size_usable();
	
	int width_bar  = 12 + 2;
	int width_pct  = 3 + 1;
	int width_free = log_u32(10, node_size_usable());
	int width_cnt  = log_u32(10, node_max_cnt());
	int width_id   = sizeof(uint32_t) * 2;
	
	int col_bar  = 0 - width_bar;
	int col_pct  = (col_bar - 1) - width_pct;
	int col_free = (col_pct - 1) - width_free;
	int col_cnt  = (col_free - 1) - width_cnt;
	int col_id   = (col_cnt - 1) - width_id;
	
	/* column headers */
	if (node->hdr.parent == 0) {
		fprintf_col(stderr, col_id, "%*s", width_id, "key.id");
		fprintf_col(stderr, col_cnt, "%*s", width_cnt, "cnt");
		fprintf_col(stderr, col_free, "%*s", width_free, "free");
		fprintf_col(stderr, col_pct, "%*s", width_pct, "used");
		
		fputc('\n', stderr);
	}
	
	/* tree graphics */
	if (level != 0) {
		for (uint32_t i = 0; i < level - 1; ++i) {
			fputs("| ", stderr);
		}
		fputs("+- ", stderr);
	}
	
	fprintf(stderr, "%s 0x%" PRIx32,
		(node->hdr.leaf ? "leaf" : "branch"), node_addr);
	
	fprintf_col(stderr, col_id, "%0*" PRIx32,
		width_id, node_first_key(node)->id);
	fprintf_col(stderr, col_cnt, "%*" PRIu16, width_cnt, node->hdr.cnt);
	fprintf_col(stderr, col_free, "%*" PRIu32, width_free, node_free(node));
	fprintf_col(stderr, col_pct, "%3d%%", (int)round(used_pct));
	fprintf_col(stderr, col_bar, "[%s]\n", used_bar);
	
	if (node->hdr.leaf) {
		/* set this only in leaves since branches are, obviously, internal */
		if (level > *max) {
			*max = level;
		}
	} else {
		branch_ptr branch = (branch_ptr)node;
		
		/* recurse through child nodes */
		const node_ref *elem_end = branch->elems + branch->hdr.cnt;
		for (const node_ref *elem = branch->elems; elem < elem_end; ++elem) {
			tree_graph_r(elem->addr, level + 1, max);
		}
	}
	
	node_unmap(node);
}

void tree_graph(uint32_t root_addr) {
	ASSERT_ROOT(root_addr);
	
	warnx("%s: root 0x%" PRIx32, __func__, root_addr);
	
	uint32_t max = 0;
	tree_graph_r(root_addr, 0, &max);
	
	warnx("%s: max depth: %" PRIu32, __func__, max + 1);
}

void tree_insert(uint32_t root_addr, const key *key, struct item_data item) {
	ASSERT_ROOT(root_addr);
	
	bool done = false, retried = false;
	do {
		leaf_ptr leaf      = tree_search(root_addr, key);
		uint32_t leaf_addr = leaf->hdr.this;
		
		if (leaf_insert(leaf, key, item)) {
			node_unmap((node_ptr)leaf);
			
			done = true;
		} else if (!retried) {
			node_unmap((node_ptr)leaf);
			node_split(leaf_addr);
			
			retried = true;
		} else {
			errx("%s: leaf_insert split ineffective: root 0x%" PRIx32
				" leaf 0x%" PRIx32 " %s len %" PRIu32,
				__func__, root_addr, leaf_addr, key_str(key), item.len);
		}
	} while (!done);
}

static leaf_ptr tree_search_r(uint32_t root_addr, uint32_t node_addr,
	const key *key) {
	node_ptr node = node_map(node_addr);
	
	/* remove this later for performance */
	check_node(node_addr, false);
	
	if (node->hdr.leaf) {
		return (leaf_ptr)node;
	} else {
		branch_ptr branch = (branch_ptr)node;
		leaf_ptr   result = NULL;
		
		ASSERT_NONEMPTY(branch);
		
		/* need a binary search that gives the element with key <= the key we
		 * are looking for, not == */
		TODO("binary search");
		
		const node_ref *elem_first = branch->elems;
		const node_ref *elem_last = branch->elems + (branch->hdr.cnt - 1);
		for (const node_ref *elem = elem_last; elem >= elem_first; --elem) {
			int8_t cmp = key_cmp(key, &elem->key);
			if (cmp > 0) {
				result = tree_search_r(root_addr, elem->addr, key);
				goto done;
			} else if (cmp == 0) {
				errx("%s: child starts with key: root 0x%" PRIx32 " node 0x%"
					PRIx32 " key %s",
					__func__, root_addr, node_addr, key_str(key));
			}
		}
		
		/* if smaller than any other key, recurse through the first subnode */
		result = tree_search_r(root_addr, elem_first->addr, key);
		
	done:
		node_unmap(node);
		
		return result;
	}
}

leaf_ptr tree_search(uint32_t root_addr, const key *key) {
	ASSERT_ROOT(root_addr);
	return tree_search_r(root_addr, root_addr, key);
}
