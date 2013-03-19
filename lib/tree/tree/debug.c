/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../tree.h"
#include <math.h>
#include "../../debug.h"


void tree_dump(uint32_t root_addr) {
	ASSERT_ROOT(root_addr);
	node_dump(root_addr, true);
}

static void tree_graph_r(uint32_t node_addr, uint32_t level,
	uint32_t *max_level, uint32_t *node_qty, uint64_t *item_qty,
	double *avg_fill) {
	node_ptr node = node_map(node_addr, false);
	
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
	int width_cnt  = log_u32(10, node_size_usable() / sizeof(item_ref));
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
	for (uint32_t i = level; i > 1; --i) {
		if (i == 2) {
			fputs("+- ", stderr);
		} else {
			fputs("| ", stderr);
		}
	}
	
	fprintf(stderr, "%s 0x%" PRIx32,
		(node->hdr.leaf ? "leaf" : "branch"), node_addr);
	
	if (node->hdr.cnt != 0) {
		fprintf_col(stderr, col_id, "%0*" PRIx32,
			width_id, node_first_key(node)->id);
	} else {
		fprintf_col(stderr, col_id, "%*s", width_id, "<empty>");
	}
	fprintf_col(stderr, col_cnt, "%*" PRIu16, width_cnt, node->hdr.cnt);
	fprintf_col(stderr, col_free, "%*" PRIu32, width_free, node_free(node));
	fprintf_col(stderr, col_pct, "%3d%%", (int)floor(used_pct));
	fprintf_col(stderr, col_bar, "[%s]\n", used_bar);
	
	if (node->hdr.leaf) {
		/* set this only in leaves since branches are, obviously, internal */
		if (level > *max_level) {
			*max_level = level;
		}
		
		*item_qty += node->hdr.cnt;
	} else {
		/* recurse through child nodes */
		const node_ref *elem_end = node->b_elems + node->hdr.cnt;
		for (const node_ref *elem = node->b_elems; elem < elem_end; ++elem) {
			tree_graph_r(elem->addr, level + 1, max_level,
				node_qty, item_qty, avg_fill);
		}
	}
	
	++(*node_qty);
	*avg_fill = (*avg_fill * ((double)(*node_qty - 1) / (double)*node_qty)) +
		(((double)node_used(node) / (double)node_size_usable()) /
		(double)*node_qty);
	
	node_unmap(node);
}

void tree_graph(uint32_t root_addr) {
	ASSERT_ROOT(root_addr);
	
	warnx("%s: root 0x%" PRIx32, __func__, root_addr);
	
	uint32_t max_level = 1;
	uint32_t node_qty = 0;
	uint64_t item_qty = 0;
	double avg_fill = 0.;
	tree_graph_r(root_addr, 1, &max_level, &node_qty, &item_qty, &avg_fill);
	
	warnx("%s: node_qty %" PRIu32 " item_qty %" PRIu64 " max_level %" PRIu32
		" avg_fill %.1f%%",
		__func__, node_qty, item_qty, max_level, avg_fill * 100.);
}
