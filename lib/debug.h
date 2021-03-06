/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#ifndef JGFS2_LIB_DEBUG_H
#define JGFS2_LIB_DEBUG_H


#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>


#define TODO(_s) \
	fprintf(stderr, "\e[37;1mTODO @ %s:%d [%s] %s\n\e[0m", \
		__FILE__, __LINE__, __func__, (_s))


struct map_node {
	struct map_node *next;
	
	struct {
		const void *addr;
		
		uint32_t sect_num;
		uint32_t sect_cnt;
	} payload;
};


static noreturn void err(const char *fmt, ...)
	__attribute__((format(printf, 1, 2)));
static noreturn void errx(const char *fmt, ...)
	__attribute__((format(printf, 1, 2)));
static void warn(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void warnx(const char *fmt, ...) __attribute__((format(printf, 1, 2)));


static noreturn void err(const char *fmt, ...) {
	fputs("jgfs2: ", stderr);
	
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	
	fprintf(stderr, ": %s\n", strerror(errno));
	
#ifdef DEBUG_FATAL
	abort();
#else
	exit(1);
#endif
}

static noreturn void errx(const char *fmt, ...) {
	fputs("jgfs2: ", stderr);
	
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	
	fputc('\n', stderr);
	
#ifdef DEBUG_FATAL
	abort();
#else
	exit(1);
#endif
}

static void warn(const char *fmt, ...) {
	fputs("jgfs2: ", stderr);
	
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	
	fprintf(stderr, ": %s\n", strerror(errno));
}

static void warnx(const char *fmt, ...) {
	fputs("jgfs2: ", stderr);
	
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	
	fputc('\n', stderr);
}


uint8_t log_u32(uint8_t base, uint32_t n);

int fprintf_col(FILE *stream, int col, const char *format, ...);

void debug_map_push(const void *addr, uint32_t sect_num, uint32_t sect_cnt);
void debug_map_pop(const void *addr, uint32_t sect_num, uint32_t sect_cnt);
void debug_map_dump(void);

void dump_mem(const void *addr, size_t len);
void dump_sect(uint32_t sect_num, uint32_t sect_cnt);
void dump_blk(uint32_t blk_num, uint32_t blk_cnt);


#endif
