#include <err.h>
#include <fuse.h>
#include <stdlib.h>
#include <string.h>


extern char *dev_path;

extern struct fuse_operations jg_oper;


int main(int argc, char **argv) {
	if (argc != 3) {
		errx(1, "expected two arguments");
	}
	
	dev_path = argv[1];
	
	/* fuse's command processing is inflexible and useless */
	int real_argc = 6;
	char **real_argv = malloc(real_argc * sizeof(char *));
	real_argv[0] = argv[0];
	real_argv[1] = strdup("-s");
	real_argv[2] = strdup("-d");
	real_argv[3] = strdup("-o");
	real_argv[4] = strdup("allow_other");
	real_argv[5] = argv[2];
	
	return fuse_main(real_argc, real_argv, &jg_oper, NULL);
}
