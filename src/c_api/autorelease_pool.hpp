#pragma once

#include "maplibre_native_c.h"

namespace mln::c_api {

#if defined(__APPLE__)

// Implemented in Objective-C++ because `@autoreleasepool` is the only pool
// construct available under ARC.
auto run_in_autorelease_pool(void* context, mln_status (*body)(void*))
  -> mln_status;

// A host frame loop that never returns to a run loop has no pool to drain
// autoreleased objects, and Metal blocks the next frame until an in-flight
// command buffer is deallocated. Drain one pool per call. Objects that cross
// the C boundary are retained, not autoreleased.
template <typename Function>
auto with_autorelease_pool(Function& function) -> mln_status {
  return run_in_autorelease_pool(&function, [](void* context) -> mln_status {
    return (*static_cast<Function*>(context))();
  });
}

#else

template <typename Function>
auto with_autorelease_pool(Function& function) -> mln_status {
  return function();
}

#endif

}  // namespace mln::c_api
