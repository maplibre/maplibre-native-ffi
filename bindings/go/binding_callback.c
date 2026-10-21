#include "binding_callback.h"

uintptr_t binding_thread(void) {
  static _Thread_local char identity;
  return (uintptr_t)&identity;
}
