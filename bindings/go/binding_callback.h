#ifndef MLN_GO_BINDING_CALLBACK_H
#define MLN_GO_BINDING_CALLBACK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
uintptr_t binding_thread(void);
static inline void* binding_address(uintptr_t value) { return (void*)value; }
// A cgo.Handle reaches native as the address of a C cell that holds it. The
// handle itself is a small integer, and Go reports a pointer-typed variable,
// such as a C struct's user_data field on a goroutine stack, that holds a
// small integer as an invalid pointer when it copies the stack.
static inline void* binding_handle_cell(uintptr_t handle) {
  uintptr_t* cell = (uintptr_t*)malloc(sizeof(uintptr_t));
  if (cell != NULL) *cell = handle;
  return cell;
}
static inline uintptr_t binding_handle_value(const void* cell) {
  return *(const uintptr_t*)cell;
}
static inline void binding_handle_free(void* cell) { free(cell); }
#endif
