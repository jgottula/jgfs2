/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "../tree.h"
#include "../../debug.h"


struct tree_lock_node *lock_list = NULL;


void tree_lock(uint32_t root_addr) {
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

void tree_unlock(uint32_t root_addr) {
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
