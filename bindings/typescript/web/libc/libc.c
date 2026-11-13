// The C library functions that the binding's C calls, for
// wasm32-unknown-unknown, which has none. The allocator is the module's own,
// through mln_web_alloc() and mln_web_free(), and snprintf() formats only the
// conversions that the binding uses: %d, %i, %u, %x, %c, %s, %p and %%, with
// the l, ll and z length modifiers, and no flags, width or precision. Rust's
// compiler_builtins defines memcpy, memmove, memset and memcmp.

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maplibre_native_c/web.h"

// Every block is aligned for any type, and its header, which keeps the size
// that mln_web_free() takes, is as large as that alignment.
#define MLN_TS_ALIGNMENT _Alignof(max_align_t)

void* malloc(size_t size) {
  if (size > SIZE_MAX - MLN_TS_ALIGNMENT) {
    return NULL;
  }
  size_t total = size + MLN_TS_ALIGNMENT;
  unsigned char* block = mln_web_alloc(total, MLN_TS_ALIGNMENT);
  if (block == NULL) {
    return NULL;
  }
  *(size_t*)block = total;
  return block + MLN_TS_ALIGNMENT;
}

void* calloc(size_t count, size_t size) {
  if (size != 0 && count > SIZE_MAX / size) {
    return NULL;
  }
  void* pointer = malloc(count * size);
  if (pointer != NULL) {
    memset(pointer, 0, count * size);
  }
  return pointer;
}

void free(void* pointer) {
  if (pointer == NULL) {
    return;
  }
  unsigned char* block = (unsigned char*)pointer - MLN_TS_ALIGNMENT;
  mln_web_free(block, *(size_t*)block, MLN_TS_ALIGNMENT);
}

void* memchr(const void* source, int value, size_t size) {
  const unsigned char* bytes = source;
  for (size_t index = 0; index < size; index++) {
    if (bytes[index] == (unsigned char)value) {
      return (void*)(bytes + index);
    }
  }
  return NULL;
}

size_t strlen(const char* text) {
  size_t length = 0;
  while (text[length] != '\0') {
    length++;
  }
  return length;
}

int strcmp(const char* left, const char* right) {
  while (*left != '\0' && *left == *right) {
    left++;
    right++;
  }
  return (unsigned char)*left - (unsigned char)*right;
}

/** Where vsnprintf() writes: the bytes that fit, and the length it counts. */
typedef struct output {
  char* buffer;
  size_t size;
  size_t length;
} output;

static void put(output* out, char character) {
  if (out->length + 1 < out->size) {
    out->buffer[out->length] = character;
  }
  out->length++;
}

static void put_text(output* out, const char* text) {
  for (; *text != '\0'; text++) {
    put(out, *text);
  }
}

static void put_unsigned(output* out, uint64_t value, unsigned base) {
  char digits[24];
  size_t count = 0;
  do {
    digits[count++] = "0123456789abcdef"[value % base];
    value /= base;
  } while (value != 0);
  while (count > 0) {
    put(out, digits[--count]);
  }
}

int vsnprintf(
  char* buffer, size_t size, const char* format, va_list arguments
) {
  output out = {.buffer = buffer, .size = size, .length = 0};
  for (; *format != '\0'; format++) {
    if (*format != '%') {
      put(&out, *format);
      continue;
    }
    format++;
    int longs = 0;
    bool sized = false;
    for (;; format++) {
      if (*format == 'l') {
        longs++;
      } else if (*format == 'z') {
        sized = true;
      } else {
        break;
      }
    }
    switch (*format) {
      case 'd':
      case 'i': {
        int64_t value = sized        ? (int64_t)va_arg(arguments, ptrdiff_t)
                        : longs >= 2 ? va_arg(arguments, long long)
                        : longs == 1 ? va_arg(arguments, long)
                                     : va_arg(arguments, int);
        if (value < 0) {
          put(&out, '-');
          put_unsigned(&out, 0 - (uint64_t)value, 10);
        } else {
          put_unsigned(&out, (uint64_t)value, 10);
        }
        break;
      }
      case 'u':
      case 'x': {
        uint64_t value = sized        ? va_arg(arguments, size_t)
                         : longs >= 2 ? va_arg(arguments, unsigned long long)
                         : longs == 1 ? va_arg(arguments, unsigned long)
                                      : va_arg(arguments, unsigned);
        put_unsigned(&out, value, *format == 'x' ? 16 : 10);
        break;
      }
      case 'c':
        put(&out, (char)va_arg(arguments, int));
        break;
      case 's': {
        const char* text = va_arg(arguments, const char*);
        put_text(&out, text != NULL ? text : "(null)");
        break;
      }
      case 'p':
        put_text(&out, "0x");
        put_unsigned(&out, (uintptr_t)va_arg(arguments, void*), 16);
        break;
      case '%':
        put(&out, '%');
        break;
      default:
        // An unsupported conversion ends the output, rather than reading
        // arguments of the wrong type.
        put_text(&out, "<unsupported format>");
        goto done;
    }
  }
done:
  if (size > 0) {
    buffer[out.length < size ? out.length : size - 1] = '\0';
  }
  return (int)out.length;
}

int snprintf(char* buffer, size_t size, const char* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  int length = vsnprintf(buffer, size, format, arguments);
  va_end(arguments);
  return length;
}
