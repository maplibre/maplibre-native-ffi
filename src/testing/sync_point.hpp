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
  // A runtime release's teardown found submission leases outstanding, such as
  // a released map's cleanup, and is about to wait for them. It fires only
  // when the teardown has to wait.
  RuntimeReleaseWaits,
  // A writer found the runtime's resource transform or resource provider
  // registration in use by a callback and is about to wait for its exclusive
  // lock: setting or clearing the registration, or runtime teardown releasing
  // it. It fires only when the writer has to wait.
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
  // A render session's driver thread has marked a driver call in flight and is
  // about to run its first work item: the core worker for each item it takes,
  // or the host's thread inside a caller-driver service call.
  RenderDriverEntered,
  // A render session's driver call has run its work, including any frame
  // result or completion that work published, and is about to end. The call
  // stays in flight until the handler returns.
  RenderDriverExited,
  // Abandoning a core-worker session found a driver call in flight and is
  // about to wait for it to end. It fires only when abandon has to wait.
  RenderAbandonWaits,
  // An Emscripten run loop's stop() has submitted its stop task, and the loop
  // may already be destroyed. Keep this point last: the suite sizes its tables
  // from it.
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
