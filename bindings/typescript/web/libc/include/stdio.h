// The part of <stdio.h> that the binding's C uses: snprintf, with the
// conversions that libc.c describes.

#ifndef MLN_TS_WEB_STDIO_H
#define MLN_TS_WEB_STDIO_H

#include <stdarg.h>
#include <stddef.h>

int snprintf(char* buffer, size_t size, const char* format, ...)
  __attribute__((format(printf, 3, 4)));
int vsnprintf(char* buffer, size_t size, const char* format, va_list arguments)
  __attribute__((format(printf, 3, 0)));

#endif
