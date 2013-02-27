/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */

#include <err.h>
#include <string.h>
#include <uuid/uuid.h>
#include "../../lib/jgfs2.h"


void jgfs2_new(const char *, const struct jgfs2_mount_options *,
	const struct jgfs2_mkfs_param *);


int main(int argc, char **argv) { // void
	struct jgfs2_mount_options mount_opt;
	mount_opt.read_only = false;
	mount_opt.debug_map = false;
	
	struct jgfs2_mkfs_param param;
	strcpy(param.label, "justix");
	uuid_generate(param.uuid);
	param.total_sect = 0;
	param.boot_sect  = 14;
	param.blk_size   = 0;
	param.zap_vbr    = true;
	param.zap_boot   = true;
	
	jgfs2_new(argv[1], &mount_opt, &param);
	jgfs2_done();
	
	warnx("success");
	
	return 0;
}
