/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */

#include "ncurses.h"


int main(int argc, char **argv) {
	/* init the jgfs2 stuff */
	
	nc_init();
	nc_loop();
	nc_done();
	
	/* deinit the jgfs2 stuff */
	
	return 0;
}
