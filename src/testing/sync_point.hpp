#pragma once

#include <atomic>
#include <cstdint>

// Named points in the library that the internal test suite can observe or park
// a thread at, for orderings that no public fence reaches.
//
// A seam here is a fallback: a test that public behavior can express belongs in
// the ABI suite instead. Every point is compiled into every build, so the code
// the internal suite tests is the code that ships. Nothing here is exported,
// and `mise run check-exports` fails if a shared library ever exports it. With
// no handler installed, hit() costs one relaxed atomic load.
namespace mln::testing {

enum class SyncPoint : std::uint8_t {
  // A map's teardown lane is about to shut down the map's worker pool, which
  // is the last cleanup a released map holds its runtime open for.
  MapPoolShutdown,
  // A writer is about to take the runtime's resource transform or resource
  // provider registration exclusively: setting or clearing it, or runtime
  // teardown releasing it. The writer waits there for in-flight callbacks.
  ResourceTransformExclusive,
  ResourceProviderExclusive,
  // A MapLibre thread holds a lease on a runtime's resource transform state,
  // with the runtime registry lock released, and is about to take the state's
  // shared lock to run the transform.
  ResourceTransformLookup,
  // Releasing a resource request, or waiting for its retirement, is about to
  // block until the request's running cancel callback returns. The request's
  // lock is held, so a handler must not park here.
  ResourceRequestCancelWait,
  // MapLibre's cancel hook for a custom provider request has returned, having
  // run the request's cancel callback when one applied.
  ResourceRequestCancelled,
  // An Emscripten run loop's stop() has submitted its stop task, and the loop
  // may already be destroyed.
  EmscriptenRunLoopStopSubmitted,
};

// Runs on whichever thread reaches the point, with no library lock held unless
// the point says otherwise. It may block to park that thread.
using SyncPointHandler = void (*)(SyncPoint point, void* context) noexcept;

namespace detail {
extern std::atomic<SyncPointHandler> sync_point_handler;
void dispatch_sync_point(SyncPoint point) noexcept;
}  // namespace detail

inline void hit(SyncPoint point) noexcept {
  if (detail::sync_point_handler.load(std::memory_order_relaxed) != nullptr)
    [[unlikely]] {
    detail::dispatch_sync_point(point);
  }
}

// Installs the process-wide handler, replacing any other; null removes it. The
// caller keeps `context` alive, and removes the handler only once no thread can
// still be inside it.
void set_sync_point_handler(SyncPointHandler handler, void* context) noexcept;

}  // namespace mln::testing
