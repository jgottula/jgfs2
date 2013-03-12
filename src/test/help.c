/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "help.h"
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include "../../lib/jgfs2.h"
#include "../../lib/tree/check.h"
#include "argp.h"


void help_init(void) {
	struct jgfs2_mount_options mount_opt = {
		.read_only = false,
		.debug_map = param.debug_map,
	};
	
	jgfs2_init(param.dev_path, &mount_opt);
}

void help_new(void) {
	struct jgfs2_mount_options mount_opt = {
		.read_only = false,
		.debug_map = param.debug_map,
	};
	struct jgfs2_mkfs_param mkfs_param = {
		.uuid = { 0 },
		
		.label = { '\0' },
		
		.total_sect = 0,
		.boot_sect  = 0,
		
		.blk_size = 0,
		
		.zap_vbr  = true,
		.zap_boot = true,
	};
	
	jgfs2_new(param.dev_path, &mount_opt, &mkfs_param);
}

bool help_check_tree(uint32_t root_addr) {
	struct check_result result = check_tree(root_addr);
	check_print(result, false);
	return (result.type == RESULT_TYPE_OK);
}
