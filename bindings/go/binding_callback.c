#include "binding_callback.h"
static _Thread_local binding_policy* current_policy;
void binding_policy_enter(binding_policy* policy) {
  policy->previous = current_policy;
  current_policy = policy;
}
void binding_policy_leave(binding_policy* policy) {
  current_policy = policy->previous;
}
bool binding_policy_check(uint32_t operation, uint64_t owner) {
  for (binding_policy* policy = current_policy; policy;
       policy = policy->previous) {
    bool allowed = false;
    if (policy->owner == owner) {
      for (size_t i = 0; i < policy->count; ++i) {
        if (policy->operations[i] == operation) {
          allowed = true;
          break;
        }
      }
    }
    if (!allowed) return false;
  }
  return true;
}
uintptr_t binding_thread(void) { return (uintptr_t)&current_policy; }
