/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include <err.h>
#include <inttypes.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "../../lib/jgfs2.h"
#include "argp.h"
#include "tests/insert.h"


unsigned long rep;


static void doing(void) {
	warnx("\e[33;1mdoing: %s %lu/%lu (seed 0x%" PRIx32 ")\e[0m",
		param.test_name, rep, param.test_rep, param.rand_seed);
}

static void success(void) {
	warnx("\e[32;1msuccess: %s %lu/%lu (seed 0x%" PRIx32 ")\e[0m",
		param.test_name, rep, param.test_rep, param.rand_seed);
}

static void failure(void) {
	warnx("\e[31;1mfailure: %s %lu/%lu (seed 0x%" PRIx32 ")\e[0m",
		param.test_name, rep, param.test_rep, param.rand_seed);
}

/* abort(3) will still dump core after we handle SIGABRT */
static void handle_sigabrt(int sig_num) {
	failure();
}

static void pre_seed(void) {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0) {
		err(1, "gettimeofday failed");
	}
	param.rand_seed = tv.tv_usec;
}

int main(int argc, char **argv) {
	signal(SIGABRT, handle_sigabrt);
	pre_seed();
	
	do_argp(argc, argv);
	
	bool (*test_func)(uint32_t) = NULL;
	if (strcasecmp(param.test_name, "insert") == 0) {
		test_func = test_insert;
	} else {
		errx(1, "test does not exist: '%s'", param.test_name);
	}
	
	/* TODO: change seed between runs */
	
	for (rep = 1; rep <= param.test_rep; ++rep) {
		doing();
		
		if (test_func(param.param0)) {
			success();
		} else {
			failure();
		}
	}
	
	warnx("all successful");
	return 0;
}
