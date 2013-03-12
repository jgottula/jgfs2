/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#ifndef JGFS2_SRC_TEST_ARGP_H
#define JGFS2_SRC_TEST_ARGP_H


#define PROG_NAME "test"


struct test_param {
	const char *dev_path;
	
	const char *test_name;
	unsigned long test_rep;
	
	uint32_t rand_seed;
	
	bool debug_map;
	
	uint32_t param0;
};
extern struct test_param param;


void do_argp(int argc, char **argv);


#endif
