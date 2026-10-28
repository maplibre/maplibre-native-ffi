#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>

#include "delivery/delivery_queue.hpp"
#include "maplibre_native_c/wake.h"

namespace mln::core {

// A host wake. One that names a queue pushes a call to the queue in place of
// each invocation, and at most one such call is pending at a time; its
// release_user_data then also runs in dispatch.
class Wake final {
 public:
  // Takes the queue that resolve_wake() resolved. Allocation failure throws.
  Wake(
    const mln_wake& descriptor,
    std::shared_ptr<delivery::DeliveryQueueState> queue
  );
  Wake(const Wake&) = delete;
  Wake(Wake&&) = delete;
  auto operator=(const Wake&) -> Wake& = delete;
  auto operator=(Wake&&) -> Wake& = delete;
  ~Wake();

  auto accept() noexcept -> void;
  auto reject() noexcept -> void;
  // Ends invocation without waiting: a notify that starts afterwards does
  // nothing. A call already running finishes, and the destructor still waits
  // for it before it releases the user data.
  auto close() noexcept -> void;
  auto notify() noexcept -> void;

 private:
  auto push_call(mln_wake_callback callback, void* user_data) noexcept -> void;

  std::mutex mutex_;
  std::condition_variable condition_;
  mln_wake descriptor_{};
  std::shared_ptr<delivery::DeliveryQueueState> queue_;
  // Shared with the queued call, which clears it before it runs.
  std::shared_ptr<std::atomic_bool> call_pending_;
  // Preallocated, so a queued wake releases its user data without allocating.
  delivery::ItemPtr release_call_;
  delivery::AbandonHold held_;
  std::size_t in_flight_ = 0;
  bool accepted_ = false;
  bool closing_ = false;
};

// Validates a wake descriptor and resolves the queue it names, if any.
auto resolve_wake(
  const mln_wake* wake, std::shared_ptr<delivery::DeliveryQueueState>& out_queue
) -> mln_status;

}  // namespace mln::core
