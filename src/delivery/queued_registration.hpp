#pragma once

#include <memory>
#include <utility>

#include "delivery/delivery_queue.hpp"
#include "maplibre_native_c.h"

namespace mln::delivery {

// Validates the queue member of a callback registration, which replaces its
// callbacks: a registration sets exactly one of them, and a queue
// registration carries no release_user_data. Resolves a queue registration's
// queue into out_queue.
auto resolve_registration_queue(
  const char* registration, mln_delivery_queue queue, bool has_callback,
  bool has_release, std::shared_ptr<DeliveryQueueState>& out_queue
) -> mln_status;

// The state of one callback registration in queue mode, which stands in for
// the host's user_data. Native code substitutes a trampoline that pushes a
// copy of each call for every callback, this state for user_data, and
// retire() for release_user_data, so the registration's own lifetime rules
// decide when it retires. A registration that is never accepted is destroyed
// without retiring.
class QueuedRegistration final {
 public:
  QueuedRegistration(std::shared_ptr<DeliveryQueueState> queue, void* user_data)
      : queue_(std::move(queue)),
        user_data_(user_data),
        retired_(make_light_item(MLN_DELIVERY_TYPE_RETIRED, user_data)) {}

  // Queues a copy of one call for the host's user_data. Returns false when
  // the copy cannot be made or the queue refuses it, and then disposes
  // nothing, so the caller still owns every handle in record.
  template <mln_delivery_type Type>
  [[nodiscard]] auto push(
    const typename Delivered<Type>::Record& record
  ) const noexcept -> bool {
    using Record = typename Delivered<Type>::Record;
    try {
      auto item = make_item(
        Type, user_data_, [&](auto& arena, mln_delivery_payload& payload) {
          payload.*Delivered<Type>::member = capture_element(arena, record);
        }
      );
      if constexpr (element_owns_handles(static_cast<const Record*>(nullptr))) {
        item->discard = [](DeliveryItem& owned) noexcept {
          discard_element(owned.view.payload.*Delivered<Type>::member);
        };
      }
      if (queue_->push(item)) return true;
      item->discard = nullptr;
    } catch (...) {
    }
    return false;
  }

  // Events share the queue tail's record and message storage.
  auto push_event(mln_runtime_event event, std::string_view message) const
    -> void {
    queue_->push_event(event, message, user_data_);
  }

  // Reserves a one-shot delivery without owned handles before accepting its
  // registration. The delivered record must fit this initial record's storage.
  template <mln_delivery_type Type>
  auto reserve(const typename Delivered<Type>::Record& record) -> void {
    using Record = typename Delivered<Type>::Record;
    static_assert(!element_owns_handles(static_cast<const Record*>(nullptr)));
    auto measure = Arena<false>{nullptr, sizeof(DeliveryItem)};
    static_cast<void>(capture_element(measure, record));
    reserved_size_ = measure.offset;
    reserved_ = ItemPtr{detail::allocate_item(reserved_size_)};
    reserved_->view.type = Type;
    reserved_->view.user_data = user_data_;
  }

  template <mln_delivery_type Type>
  auto push_reserved(const typename Delivered<Type>::Record& record) noexcept
    -> void {
    auto arena = Arena<true>{
      reinterpret_cast<std::byte*>(reserved_.get()), sizeof(DeliveryItem),
      reserved_size_
    };
    reserved_->view.payload.*Delivered<Type>::member =
      capture_element(arena, record);
    static_cast<void>(queue_->push(reserved_));
  }

  // The release_user_data of a queue registration: pushes
  // MLN_DELIVERY_TYPE_RETIRED after the last delivery, as callback mode
  // releases user_data after the last call, and frees the state.
  static auto retire(void* registration) noexcept -> void {
    const auto owned = std::unique_ptr<QueuedRegistration>{
      static_cast<QueuedRegistration*>(registration)
    };
    static_cast<void>(owned->queue_->push(owned->retired_));
  }

 private:
  std::shared_ptr<DeliveryQueueState> queue_;
  void* user_data_ = nullptr;
  // Preallocated, so retirement reaches a live queue without allocating.
  ItemPtr retired_;
  ItemPtr reserved_;
  std::size_t reserved_size_ = 0;
};

}  // namespace mln::delivery
