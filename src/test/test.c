/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include <err.h>
#include <stdlib.h>
#include "rand.h"


extern void test_mmap();
extern void test_tree0();
extern void test_tree1();

int main(int argc, char **argv) {
	warnx("performing unit tests");
	
	/* constant seed value so tests are repeatable */
	srand48(0);
	
	//test_mmap();
	//test_tree0();
	test_tree1();
}
