#ifndef JGFS2_LIB_DEBUG_H
#define JGFS2_LIB_DEBUG_H


#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define TODO(_s) \
	fprintf(stderr, "TODO: %s: %s\n", __func__, (_s))


static void err(int eval, const char *fmt, ...) {
	fputs("jgfs2: ", stderr);
	
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	
	fprintf(stderr, ": %s\n", strerror(errno));
	
	exit(eval);
}

static void errx(int eval, const char *fmt, ...) {
	fputs("jgfs2: ", stderr);
	
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	
	fputc('\n', stderr);
	
	exit(eval);
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


#endif
