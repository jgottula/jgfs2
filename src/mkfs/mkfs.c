#include <err.h>
#include <uuid/uuid.h>
#include "../../lib/jgfs2.h"


void jgfs2_new(const char *, const struct jgfs2_mount_options *,
	const struct jgfs2_mkfs_param *);


int main(int argc, char **argv) { // void
	struct jgfs2_mount_options mount_opt;
	mount_opt.read_only = false;
	
	struct jgfs2_mkfs_param param;
	strcpy(param.label, "justix");
	uuid_generate(param.uuid);
	param.sect_count = 0;
	param.boot_size  = 16;
	param.block_size = 0;
	param.zap_vbr    = true;
	param.zap_boot   = true;
	param.zap_data   = false;
	
	jgfs2_new(argv[1], &mount_opt, &param);
	
	errx(1, "not implemented");
}
