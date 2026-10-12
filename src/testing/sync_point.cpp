#include "testing/sync_point.hpp"

namespace mln::testing {

namespace detail {

std::atomic<SyncPointHandler> sync_point_handler{nullptr};

namespace {
std::atomic<void*> sync_point_context{nullptr};
}  // namespace

void dispatch_sync_point(SyncPoint point) noexcept {
  // The acquire pairs with the release in set_sync_point_handler(), so the
  // context a handler reads is the one installed with it.
  const auto handler = sync_point_handler.load(std::memory_order_acquire);
  if (handler != nullptr) {
    handler(point, sync_point_context.load(std::memory_order_relaxed));
  }
}

}  // namespace detail

void set_sync_point_handler(SyncPointHandler handler, void* context) noexcept {
  detail::sync_point_handler.store(nullptr, std::memory_order_release);
  detail::sync_point_context.store(context, std::memory_order_relaxed);
  detail::sync_point_handler.store(handler, std::memory_order_release);
}

}  // namespace mln::testing
