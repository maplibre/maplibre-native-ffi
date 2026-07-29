#pragma once

#include "maplibre_native_c.h"

namespace mln::c_api {

#if defined(__APPLE__)

// Implemented in Objective-C++ because `@autoreleasepool` is the only pool
// construct available under ARC. The body crosses as a plain function pointer
// so C++ translation units can drain a pool without being Objective-C++
// themselves.
auto run_in_autorelease_pool(void* context, mln_status (*body)(void*))
  -> mln_status;

// Apple frameworks return autoreleased objects, and a host frame loop that
// never returns to a run loop has no pool to drain them. Metal in particular
// caps the command buffers a queue may have in flight and frees a slot only
// once a buffer is deallocated, so an undrained pool blocks the next frame
// forever. Draining one pool per call keeps that bounded no matter how the
// host drives the library.
//
// Objects that cross the C boundary are retained rather than autoreleased, so
// draining here cannot outlive a value the caller still holds.
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
