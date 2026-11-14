// The part of <string.h> that the binding's C uses. wasm32-unknown-unknown has
// no C library: Rust's compiler_builtins defines the mem* functions, and
// libc.c the rest.

#ifndef MLN_TS_WEB_STRING_H
#define MLN_TS_WEB_STRING_H

#include <stddef.h>

void* memcpy(void* destination, const void* source, size_t size);
void* memmove(void* destination, const void* source, size_t size);
void* memset(void* destination, int value, size_t size);
int memcmp(const void* left, const void* right, size_t size);
void* memchr(const void* source, int value, size_t size);
size_t strlen(const char* text);
int strcmp(const char* left, const char* right);

#endif
