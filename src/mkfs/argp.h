/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#ifndef JGFS2_SRC_MKFS_ARGP_H
#define JGFS2_SRC_MKFS_ARGP_H


#define PROG_NAME "mkfs"


extern const char *dev_path;
extern struct jgfs2_mount_options mount_opt;
extern struct jgfs2_mkfs_param param;


void do_argp(int argc, char **argv);


#endif
