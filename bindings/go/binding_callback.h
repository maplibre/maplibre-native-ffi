#ifndef MLN_GO_BINDING_CALLBACK_H
#define MLN_GO_BINDING_CALLBACK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
typedef struct binding_policy {
  struct binding_policy* previous;
  const uint32_t* operations;
  size_t count;
  uint64_t owner;
} binding_policy;
void binding_policy_enter(binding_policy* policy);
void binding_policy_leave(binding_policy* policy);
bool binding_policy_check(uint32_t operation, uint64_t owner);
uintptr_t binding_thread(void);
// Refuses every native call on the calling thread until the matching leave.
void binding_report_enter(void);
void binding_report_leave(void);
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
