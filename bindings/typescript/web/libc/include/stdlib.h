// The part of <stdlib.h> that the binding's C uses, which libc.c defines over
// the module's allocator.

#ifndef MLN_TS_WEB_STDLIB_H
#define MLN_TS_WEB_STDLIB_H

#include <stddef.h>

void* malloc(size_t size);
void* calloc(size_t count, size_t size);
void free(void* pointer);

#endif
