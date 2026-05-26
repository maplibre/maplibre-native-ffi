#ifndef MLN_GO_INTERNAL_CGO_SHIM_H
#define MLN_GO_INTERNAL_CGO_SHIM_H

#include <stdint.h>

#include "maplibre_native_c.h"

static inline void* mln_go_handle_to_pointer(uintptr_t handle) {
  return (void*)handle;
}

#endif
