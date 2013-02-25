#include "../../lib/jgfs2.h"
#include "../../lib/debug.h"
#include "../../lib/fs.h"
#include "../../lib/tree.h"


static void dump_and_check(void) {
	node_dump(fs.sblk->s_addr_meta_tree);
	check_print(check_node(fs.sblk->s_addr_meta_tree), true);
}

void test_tree(void) {
	fprintf(stderr, "%s\n", __func__);
	
	jgfs2_init("/dev/loop0p1",
		&(struct jgfs2_mount_options){
			.read_only = false,
			.debug_map = true,
		});
	dump_and_check();
	
	struct inode_item inode;
	memset(&inode, 0xff, sizeof(inode));
	key key = {
		lrand48(),
		KEY_INODE,
		0,
	};
	tree_insert(fs.sblk->s_addr_meta_tree, &key,
		(struct item_data){ sizeof(inode), &inode });
	dump_and_check();
	
	// do an operation
	// then dump memory
	// then check the entire tree
	
	// then, do another operation...
	
	jgfs2_done();
}
