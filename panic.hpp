#ifndef PANIC_HEADER
#define PANIC_HEADER

#include <stdarg.h>

static void panic(const char *fmt, ...){
	va_list arg;

	va_start(arg, fmt);
	fputs("Error: ", stderr);
	vfprintf(stderr, fmt, arg);
	va_end(arg);
	fputs("\n", stderr);

	exit(1);
}

#endif // PANIC_HEADER
