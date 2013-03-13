/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "argp.h"
#include <argp.h>
#include <bsd/string.h>
#include <err.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>
#include "../../lib/jgfs2.h"


/* configurable parameters */
const char *dev_path = NULL;
struct jgfs2_mount_options mount_opt = {
	.read_only = false,
	.debug_map = false,
};
struct jgfs2_mkfs_param param = {
	.uuid  = { 0 },
	
	.label = "",
	
	.total_sect = 0,  // auto
	.boot_sect  = 14,
	
	.blk_size   = 0,  // auto
	
	.zap_vbr    = false,
	.zap_boot   = false,
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
	case 'U':
		if (uuid_parse(arg, param.uuid) != 0) {
			warnx("uuid: don't understand '%s'", arg);
			argp_usage(state);
		}
		break;
	case 'L':
		if (strlen(arg) > JGFS2_LIMIT_LABEL) {
			warnx("truncating volume label to %u characters",
				JGFS2_LIMIT_LABEL);
		}
		strlcpy(param.label, arg, JGFS2_LIMIT_LABEL + 1);
		break;
		
	case 's':
		switch (sscanf(arg, "%" SCNu32, &param.total_sect)) {
		case EOF:
		default:
			warnx("total_sect: don't understand '%s'", arg);
			argp_usage(state);
		case 1:
			break;
		}
		break;
	case 'b':
		switch (sscanf(arg, "%" SCNu16, &param.boot_sect)) {
		case EOF:
			warnx("boot_sect: don't understand '%s'", arg);
			argp_usage(state);
		case 1:
			break;
		}
		break;
	case 'B':
		switch (sscanf(arg, "%" SCNu16, &param.blk_size)) {
		case EOF:
			warnx("blk_size: don't understand '%s'", arg);
			argp_usage(state);
		case 1:
			break;
		}
		break;
	
	case 'z':
	{
		size_t tok_num = 0;
		char *tok = strtok(arg, ",");
		while (tok != NULL) {
			if (strcasecmp(tok, "vbr") == 0) {
				param.zap_vbr = true;
				++tok_num;
			} else if (strcasecmp(tok, "boot") == 0) {
				param.zap_boot = true;
				++tok_num;
			} else {
				warnx("zap: don't understand '%s'", tok);
				argp_usage(state);
			}
			
			tok = strtok(NULL, ",");
		}
		if (tok_num == 0) {
			warnx("zap: no areas given");
			argp_usage(state);
		}
		break;
	}
	
	case 'D':
	{
		size_t tok_num = 0;
		char *tok = strtok(arg, ",");
		while (tok != NULL) {
			if (strcasecmp(tok, "map") == 0) {
				mount_opt.debug_map = true;
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
			dev_path = strdup(arg);
		} else {
			warnx("excess argument: '%s'", arg);
			argp_usage(state);
		}
		break;
	case ARGP_KEY_END:
		if (state->arg_num < 1) {
			warnx("device not specified");
			argp_usage(state);
		}
		break;
	
	default:
		return ARGP_ERR_UNKNOWN;
	}
	
	return 0;
}


/* argp structures */
static const char args_doc[] = "DEVICE";
static const char doc[] = "Initialize jgfs2 on a file or block device.";
static struct argp_option options[] = {
	{ NULL, 0, NULL, 0, "filesystem ID parameters:", 1 },
	{ "uuid", 'U', "UUID", 0, NULL, 1 },
	{ "label", 'L', "STRING", 0, NULL, 1 },
	
	{ NULL, 0, NULL, 0, "size parameters:", 2 },
	{ "size", 's', "SECTORS", 0, NULL, 2, },
	{ "boot", 'b', "SECTORS", 0, NULL, 2, },
	{ "blk-size", 'B', "SECTORS", 0, NULL, 2, },
	
	{ NULL, 0, NULL, 0, "initialization options:", 3 },
	{ "zap", 'z', "AREAS", 0, NULL, 3 },
	
	{ NULL, 0, NULL, 0, "debug options:", 4 },
	{ "debug", 'D', "FLAGS", 0, NULL, 4 },
	
	{ 0 }
};
static struct argp argp =
	{ options, &parse_opt, args_doc, doc, NULL, NULL, NULL };
void (*argp_program_version_hook)(FILE *stream, struct argp_state *state) =
	print_version;


static void set_docs(void) {
	struct argp_option opt_zero;
	memset(&opt_zero, 0, sizeof(opt_zero));
	
	struct argp_option *opt = options;
	while (memcmp(opt, &opt_zero, sizeof(*opt)) != 0) {
		switch (opt->key) {
		case 'U':
			opt->doc =
				"universally unique identifier\n"
				"> default: random";
			break;
		case 'L':
			opt->doc = sprintf_alloc(
				"volume label [0-%u chars]\n"
				"> default: '%s'",
				JGFS2_LIMIT_LABEL, param.label);
			break;
		
		case 's':
			opt->doc =
				"filesystem size\n"
				"> default: fill device";
			break;
		case 'b':
			opt->doc = sprintf_alloc(
				"boot area size [0-%" PRIu16 "]\n"
				"> default: %" PRIu16,
				UINT16_MAX, param.boot_sect);
			break;
		case 'B':
			opt->doc =
				"block size\n"
				"> default: auto";
			break;
		
		case 'z':
			opt->doc =
				"zero-fill device area(s)\n"
				"> areas: vbr, boot";
			break;
		
		case 'D':
			opt->doc =
				"enable debug flags\n"
				"> flags: map";
			break;
		}
		
		++opt;
	}
}

void do_argp(int argc, char **argv) {
	set_docs();
	argp_parse(&argp, argc, argv, 0, NULL, NULL);
}
