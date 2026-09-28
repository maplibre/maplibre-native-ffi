#ifndef MLN_GO_BINDING_CALLBACK_H
#define MLN_GO_BINDING_CALLBACK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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
static inline void* binding_address(uintptr_t value) { return (void*)value; }
#endif
