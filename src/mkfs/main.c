/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include <err.h>
#include "../../lib/jgfs2.h"
#include "argp.h"


int main(int argc, char **argv) {
	do_argp(argc, argv);
	
	jgfs2_new(dev_path, &mount_opt, &param);
	warnx("TODO: report statistics");
	jgfs2_done();
	
	warnx("success");
	return 0;
}
