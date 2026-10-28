#pragma once

#include <array>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "delivery/delivery_queue.hpp"
#include "maplibre_native_c.h"

namespace mln::delivery {

// Validates the queue member of a callback registration. A registration sets
// a callback, a queue, or both. A queue alone drains records and carries no
// release_user_data; a callback with a queue dispatches calls. Resolves a
// named queue into out_queue.
auto resolve_registration_queue(
  const char* registration, mln_delivery_queue queue, bool has_callback,
  bool has_release, std::shared_ptr<DeliveryQueueState>& out_queue
) -> mln_status;

// The state of one callback registration that names a queue, which stands in
// for the host's user_data. Native code substitutes a trampoline that pushes
// a copy of each call for every callback, this state for user_data, and
// retire() for release_user_data, so the registration's own lifetime rules
// decide when it retires. A registration that is never accepted is destroyed
// without retiring.
//
// A drained registration pushes records and retires with
// MLN_DELIVERY_TYPE_RETIRED. A dispatched one pushes calls to its callbacks
// and retires with a call to its release_user_data.
class QueuedRegistration final {
 public:
  // One host callback of a dispatched registration, by the delivery type
  // that copies its arguments.
  struct Callback {
    mln_delivery_type type;
    HostFunction function;
  };

  template <mln_delivery_type Type>
  static auto callback(typename Delivered<Type>::Callback function) noexcept
    -> Callback {
    return Callback{
      .type = Type, .function = reinterpret_cast<HostFunction>(function)
    };
  }

  // A registration with no non-null callback drains.
  QueuedRegistration(
    std::shared_ptr<DeliveryQueueState> queue, void* user_data,
    mln_user_data_release release = nullptr,
    std::initializer_list<Callback> callbacks = {}
  )
      : queue_(std::move(queue)), user_data_(user_data) {
    for (const auto& entry : callbacks) {
      if (entry.function == nullptr) continue;
      callbacks_.at(entry.type) = entry.function;
      dispatched_ = true;
    }
    if (!dispatched_) {
      retired_ = make_light_item(MLN_DELIVERY_TYPE_RETIRED, user_data);
    } else if (release != nullptr) {
      retired_ = make_release_call(user_data, release);
    }
  }

  // Queues a copy of one call for the host's user_data. Returns false when
  // the copy cannot be made or the queue refuses it, and then disposes
  // nothing, so the caller still owns every handle in record. A dispatched
  // registration without a callback for Type drops the call.
  template <mln_delivery_type Type>
  [[nodiscard]] auto push(
    const typename Delivered<Type>::Record& record
  ) const noexcept -> bool {
    using Record = typename Delivered<Type>::Record;
    if (dispatched_ && callbacks_[Type] == nullptr) {
      discard_element(record);
      return true;
    }
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
      make_call<Type>(*item);
      if (queue_->push(item)) return true;
      item->discard = nullptr;
    } catch (...) {
    }
    return false;
  }

  // Events share the queue tail's record and message storage.
  auto push_event(
    mln_runtime_event event, std::string_view message,
    std::vector<std::string> strings = {}
  ) const -> void {
    queue_->push_event(
      event, message, user_data_, std::move(strings),
      callbacks_[MLN_DELIVERY_TYPE_RUNTIME_EVENTS]
    );
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
    make_call<Type>(*reserved_);
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

  // The release_user_data of a queue registration: queues the retirement
  // after the last call, as callback mode releases user_data after the last
  // call, and frees the state. A refused retirement abandons user_data.
  static auto retire(void* registration) noexcept -> void {
    const auto owned = std::unique_ptr<QueuedRegistration>{
      static_cast<QueuedRegistration*>(registration)
    };
    if (owned->retired_ == nullptr) return;
    static_cast<void>(owned->queue_->push(owned->retired_));
  }

 private:
  template <mln_delivery_type Type>
  auto make_call(DeliveryItem& item) const noexcept -> void {
    if (!dispatched_) return;
    item.dispatched = true;
    item.callback = callbacks_[Type];
    item.invoke = &invoke_delivered<Type>;
  }

  std::shared_ptr<DeliveryQueueState> queue_;
  void* user_data_ = nullptr;
  // A dispatched registration's callbacks, by delivery type.
  std::array<HostFunction, MLN_DELIVERY_TYPE_RESOURCE_TRANSFORM + 1>
    callbacks_{};
  bool dispatched_ = false;
  // Preallocated, so retirement reaches a live queue without allocating.
  // Null for a dispatched registration without release_user_data.
  ItemPtr retired_;
  ItemPtr reserved_;
  std::size_t reserved_size_ = 0;
};

}  // namespace mln::delivery
