/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "argp.h"
#include <argp.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include "../../lib/jgfs2.h"


/* configurable parameters */
struct test_param param = {
	.dev_path = NULL,
	
	.test_name = NULL,
	.test_rep  = 1,
	
	.rand_seed = 0,
	
	.debug_map = false,
	
	.param0 = 0,
};


static char *sprintf_alloc(const char *format, ...) {
	static const size_t buf_size = 0x10000;
	char *buf = malloc(buf_size);
	
	va_list ap;
	va_start(ap, format);
	int final_size = vsnprintf(buf, buf_size, format, ap);
	va_end(ap);
	
	return realloc(buf, final_size + 1);
}

static void print_version(FILE *stream, struct argp_state *state) {
	fprintf(stream, PROG_NAME ".jgfs2 0x%04x\n", JGFS2_VER_TOTAL);
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	switch (key) {
	case 's':
		switch (sscanf(arg, "0x%" SCNx32, &param.rand_seed)) {
		case EOF:
		default:
			warnx("rand_seed: don't understand '%s'", arg);
			argp_usage(state);
		case 1:
			break;
		}
		break;
	
	case 'D':
	{
		size_t tok_num = 0;
		char *tok = strtok(arg, ",");
		while (tok != NULL) {
			if (strcasecmp(tok, "map") == 0) {
				param.debug_map = true;
				++tok_num;
			} else {
				warnx("debug: don't understand '%s'", tok);
				argp_usage(state);
			}
			
			tok = strtok(NULL, ",");
		}
		if (tok_num == 0) {
			warnx("debug: no flags given");
			argp_usage(state);
		}
		break;
	}
	
	case ARGP_KEY_ARG:
		if (state->arg_num == 0) {
			param.dev_path = strdup(arg);
		} else if (state->arg_num == 1) {
			param.test_name = strdup(arg);
		} else if (state->arg_num == 2) {
			switch (sscanf(arg, "%lu", &param.test_rep)) {
			case EOF:
			default:
				warnx("test_rep: don't understand '%s'", arg);
				argp_usage(state);
			case 1:
				break;
			}
		} else if (state->arg_num == 3) {
			switch (sscanf(arg, "%" SCNu32, &param.param0)) {
			case EOF:
			default:
				warnx("param0: don't understand '%s'", arg);
				argp_usage(state);
			case 1:
				break;
			}
		} else {
			warnx("excess argument: '%s'", arg);
			argp_usage(state);
		}
		break;
	case ARGP_KEY_END:
		if (state->arg_num < 1) {
			warnx("device not specified");
			argp_usage(state);
		} else if (state->arg_num < 2) {
			warnx("test not specified");
			argp_usage(state);
		} else if (state->arg_num < 3) {
			warnx("reps not specified");
			argp_usage(state);
		}
		break;
	
	default:
		return ARGP_ERR_UNKNOWN;
	}
	
	return 0;
}


/* argp structures */
static const char args_doc[] = "DEVICE TEST REPS PARAM0";
static const char doc[] = "Run jgfs2 unit tests on a device.";
static struct argp_option options[] = {
	{ NULL, 0, NULL, 0, "RNG parameters:", 1 },
	{ "seed", 's', "UINT32", 0,
		"random seed\n> format: 0x...", 1, },
	
	{ NULL, 0, NULL, 0, "debug options:", 2 },
	{ "debug", 'D', "FLAGS", 0,
		"enable debug flags\n> flags: map", 2 },
	
	{ 0 }
};
static struct argp argp =
	{ options, &parse_opt, args_doc, doc, NULL, NULL, NULL };
void (*argp_program_version_hook)(FILE *stream, struct argp_state *state) =
	print_version;


void do_argp(int argc, char **argv) {
	argp_parse(&argp, argc, argv, 0, NULL, NULL);
}
