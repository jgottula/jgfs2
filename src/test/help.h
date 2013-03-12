/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#ifndef JGFS2_SRC_TEST_HELP_H
#define JGFS2_SRC_TEST_HELP_H


#define FAIL_ON(_cond) \
	if (!(_cond)) { return false; }


void help_init(void);
void help_new(void);

bool help_check_tree(uint32_t root_addr);


#endif
