/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "tree.h"
#include <math.h>
#include "../debug.h"
#include "check.h"


struct tree_lock_node *lock_list = NULL;


static void tree_lock(uint32_t root_addr) {
	struct tree_lock_node *node = lock_list;
	while (node != NULL) {
		if (node->root_addr == root_addr) {
			errx("%s: tree 0x%" PRIx32 " already locked", __func__, root_addr);
		}
		
		node = node->next;
	}
	
	struct tree_lock_node *new = malloc(sizeof(struct tree_lock_node));
	new->next = lock_list;
	lock_list = new;
	
	new->root_addr = root_addr;
}

static void tree_unlock(uint32_t root_addr) {
	struct tree_lock_node **prev = &lock_list, *node = lock_list;
	while (node != NULL) {
		if (node->root_addr == root_addr) {
			*prev = node->next;
			free(node);
			
			return;
		}
		
		prev = &node->next;
		node = node->next;
	}
	
	errx("%s: tree 0x%" PRIx32 " was not locked", __func__, root_addr);
}

void tree_init(uint32_t root_addr) {
	tree_lock(root_addr);
	node_unmap((node_ptr)leaf_init(root_addr, 0, 0, 0));
	tree_unlock(root_addr);
}

void tree_dump(uint32_t root_addr) {
	ASSERT_ROOT(root_addr);
	tree_lock(root_addr);
	node_dump(root_addr, true);
	tree_unlock(root_addr);
}

static void tree_graph_r(uint32_t node_addr, uint32_t level,
	uint32_t *max_level, uint32_t *node_qty, uint64_t *item_qty,
	double *avg_fill) {
	node_ptr node = node_map(node_addr);
	
	/* draw the node space usage bar */
	uint8_t used_round = (uint8_t)ceil(((float)node_used(node_addr) * 12.f) /
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
	
	float used_pct = ((float)node_used(node_addr) * 100.f) /
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
	fprintf_col(stderr, col_free, "%*" PRIu32,
		width_free, node_free(node_addr));
	fprintf_col(stderr, col_pct, "%3d%%", (int)round(used_pct));
	fprintf_col(stderr, col_bar, "[%s]\n", used_bar);
	
	if (node->hdr.leaf) {
		/* set this only in leaves since branches are, obviously, internal */
		if (level > *max_level) {
			*max_level = level;
		}
		
		*item_qty += node->hdr.cnt;
	} else {
		branch_ptr branch = (branch_ptr)node;
		
		/* recurse through child nodes */
		const node_ref *elem_end = branch->elems + branch->hdr.cnt;
		for (const node_ref *elem = branch->elems; elem < elem_end; ++elem) {
			tree_graph_r(elem->addr, level + 1, max_level,
				node_qty, item_qty, avg_fill);
		}
	}
	
	++(*node_qty);
	*avg_fill = (*avg_fill * ((double)(*node_qty - 1) / (double)*node_qty)) +
		(((double)node_used(node_addr) / (double)node_size_usable()) /
		(double)*node_qty);
	
	node_unmap(node);
}

void tree_graph(uint32_t root_addr) {
	ASSERT_ROOT(root_addr);
	tree_lock(root_addr);
	
	warnx("%s: root 0x%" PRIx32, __func__, root_addr);
	
	uint32_t max_level = 1;
	uint32_t node_qty = 0;
	uint64_t item_qty = 0;
	double avg_fill = 0.;
	tree_graph_r(root_addr, 1, &max_level, &node_qty, &item_qty, &avg_fill);
	
	warnx("%s: node_qty %" PRIu32 " item_qty %" PRIu64 " max_level %" PRIu32
		" avg_fill %.1f%%",
		__func__, node_qty, item_qty, max_level, avg_fill * 100.);
	
	tree_unlock(root_addr);
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
	
	tree_lock(root_addr);
	leaf_ptr result = tree_search_r(root_addr, root_addr, key);
	tree_unlock(root_addr);
	
	return result;
}

bool tree_retrieve(uint32_t root_addr, const key *key, struct item_data *item) {
	ASSERT_ROOT(root_addr);
	tree_lock(root_addr);
	
	bool result = false;
	leaf_ptr leaf = tree_search_r(root_addr, root_addr, key);
	uint16_t idx;
	if (node_search((node_ptr)leaf, key, &idx)) {
		item_ref *elem = leaf->elems + idx;
		
		item->len  = elem->len;
		item->data = leaf_data_ptr(leaf, elem);
		
		result = true;
	}
	
	node_unmap((node_ptr)leaf);
	tree_unlock(root_addr);
	
	return result;
}

void tree_insert(uint32_t root_addr, const key *key, struct item_data item) {
	ASSERT_ROOT(root_addr);
	tree_lock(root_addr);
	
	bool done = false;
	uint8_t retry = 0;
	do {
		/* use tree_search_r, not tree_search, to circumvent locking */
		leaf_ptr leaf      = tree_search_r(root_addr, root_addr, key);
		uint32_t leaf_addr = leaf->hdr.this;
		
		if (leaf_insert(leaf, key, item)) {
			node_unmap((node_ptr)leaf);
			
			done = true;
		} else if (retry == 0) {
			node_unmap((node_ptr)leaf);
			node_split(leaf_addr);
			
			++retry;
		} else {
			errx("%s: giving up: root 0x%" PRIx32 " leaf 0x%"
				PRIx32 " %s len %" PRIu32,
				__func__, root_addr, leaf_addr, key_str(key), item.len);
		}
	} while (!done);
	
	tree_unlock(root_addr);
}
