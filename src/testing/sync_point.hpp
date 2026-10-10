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
  // block until the request's running cancel callback returns, or until
  // another release finishes releasing the user data of a registration whose
  // callback never ran. The request's lock is held, so a handler must not park
  // here.
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
  // A render session's driver call has published a frame demand's result,
  // which the host can drain, and is about to finish the demand. The call
  // stays in flight until the handler returns. No lock is held.
  RenderFrameResultPublished,
  // Abandoning a core-worker session found a driver call in flight and is
  // about to wait for it to end. It fires only when abandon has to wait.
  RenderAbandonWaits,
  // A detach has marked its session detaching and queued the work that
  // detaches it, and is about to return. No lock is held, so an abandon can
  // run while a detaching thread is parked here.
  RenderDetachQueued,
  // A disposed core-worker session's worker has detached the session, freeing
  // its graphics objects, and is about to hand the session to its teardown
  // lane. It fires only when disposal detaches rather than quarantines. No
  // lock is held.
  RenderDisposalDetached,
  // A map's run loop handed MapLibre a finished frame of an update older than
  // the map's pending still-image request, reported as partial so that it
  // cannot complete the image.
  StillImageFrameHeldBack,
  // A run loop in src/platform/run_loop has submitted its stop task from
  // stop(), and the loop may already be destroyed.
  RunLoopStopSubmitted,
  // A GeoJSON data's sequenced worker holds the data and is about to slice one
  // tile for an asynchronous request. Synchronous tiling slices inline and
  // never reaches it. No lock is held, so a parked worker delays only the
  // slices queued behind it on the same worker.
  GeoJsonTileSlice,
  // A standalone projection call has leased its handle and is about to take
  // the projection's call lock. No lock is held.
  ProjectionCallLeased,
  // A standalone projection call holds the projection's call lock and is
  // about to run. Parking here blocks only the calls, and the close, of that
  // one projection.
  ProjectionCallRunning,
  // A projection close retired the handle and found a call running, and is
  // about to wait for it. It fires only when the close has to wait. Keep this
  // point last: the suite sizes its tables from it.
  ProjectionCloseWaits,
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
