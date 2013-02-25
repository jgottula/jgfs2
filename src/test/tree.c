#include "../../lib/jgfs2.h"
#include "../../lib/debug.h"
#include "../../lib/fs.h"
#include "../../lib/tree.h"


static void dump_and_check(void) {
	node_dump(fs.sblk->s_addr_meta_tree);
	
	struct check_result result = check_node(fs.sblk->s_addr_meta_tree);
	check_print(&result, true);
}

void test_tree(void) {
	fprintf(stderr, "%s", __func__);
	
	jgfs2_init("/dev/loop0p1",
		&(struct jgfs2_mount_options){ .read_only = false });
	
	dump_and_check();
	
	// do an operation
	// then dump memory
	// then check the entire tree
	
	// then, do another operation...
	
	jgfs2_done();
}
