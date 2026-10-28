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

// Validates the callback and queue members of a callback registration, and
// resolves a named queue into out_queue.
auto resolve_registration_queue(
  const char* registration, mln_delivery_queue queue, bool has_callback,
  std::shared_ptr<DeliveryQueueState>& out_queue
) -> mln_status;

// The state of one callback registration that names a queue, which stands in
// for the host's user_data. Native code substitutes a trampoline that pushes
// a copy of each call for every callback, this state for user_data, and
// retire() for release_user_data, so the registration's own lifetime rules
// decide when it retires. A registration that is never accepted is destroyed
// without retiring.
//
// Each call runs its host callback in dispatch, and retirement queues a call
// to the host's release_user_data.
class QueuedRegistration final {
 public:
  // One host callback, by the record that copies its arguments.
  struct Callback {
    std::size_t index;
    HostFunction function;
  };

  template <class Record>
  static auto callback(typename Delivered<Record>::Callback function) noexcept
    -> Callback {
    return Callback{
      .index = Delivered<Record>::index,
      .function = reinterpret_cast<HostFunction>(function),
    };
  }

  QueuedRegistration(
    std::shared_ptr<DeliveryQueueState> queue, void* user_data,
    mln_user_data_release release, std::initializer_list<Callback> callbacks
  )
      : queue_(std::move(queue)), user_data_(user_data) {
    for (const auto& entry : callbacks) {
      callbacks_.at(entry.index) = entry.function;
    }
    if (release != nullptr) {
      retired_ = make_release_call(user_data, release);
    }
  }

  // Queues a copy of one call for the host's user_data. Returns false when
  // the copy cannot be made or the queue refuses it, and then disposes
  // nothing, so the caller still owns every handle in record. A registration
  // without a callback for Record drops the call.
  template <class Record>
  [[nodiscard]] auto push(const Record& record) const noexcept -> bool {
    const auto callback = callbacks_[Delivered<Record>::index];
    if (callback == nullptr) {
      discard_element(record);
      return true;
    }
    try {
      auto item = make_item(user_data_, record);
      if constexpr (element_owns_handles(static_cast<const Record*>(nullptr))) {
        item->discard = [](DeliveryItem& owned) noexcept {
          discard_element(*static_cast<const Record*>(owned.arguments));
        };
      }
      item->callback = callback;
      item->invoke = &invoke_delivered<Record>;
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
      event, message, user_data_,
      reinterpret_cast<mln_runtime_event_callback>(
        callbacks_[Delivered<RuntimeEventArguments>::index]
      ),
      std::move(strings)
    );
  }

  // Reserves a one-shot call without owned handles before accepting its
  // registration. The pushed record must fit this initial record's storage.
  template <class Record>
  auto reserve(const Record& record) -> void {
    static_assert(!element_owns_handles(static_cast<const Record*>(nullptr)));
    auto measure = Arena<false>{nullptr, sizeof(DeliveryItem)};
    static_cast<void>(measure.template allocate<Record>(1));
    static_cast<void>(capture_element(measure, record));
    reserved_size_ = measure.offset;
    reserved_ = ItemPtr{detail::allocate_item(reserved_size_)};
    reserved_->user_data = user_data_;
    reserved_->callback = callbacks_[Delivered<Record>::index];
    reserved_->invoke = &invoke_delivered<Record>;
  }

  template <class Record>
  auto push_reserved(const Record& record) noexcept -> void {
    auto arena = Arena<true>{
      reinterpret_cast<std::byte*>(reserved_.get()), sizeof(DeliveryItem),
      reserved_size_
    };
    auto* copy = arena.template allocate<Record>(1);
    *copy = capture_element(arena, record);
    reserved_->arguments = copy;
    static_cast<void>(queue_->push(reserved_));
  }

  // The release_user_data of a queue registration: queues the call to the
  // host's release_user_data after the last call, as callback mode releases
  // user_data after the last call, and frees the state. A refused retirement
  // abandons user_data.
  static auto retire(void* registration) noexcept -> void {
    const auto owned = std::unique_ptr<QueuedRegistration>{
      static_cast<QueuedRegistration*>(registration)
    };
    if (owned->retired_ == nullptr) return;
    static_cast<void>(owned->queue_->push(owned->retired_));
  }

 private:
  std::shared_ptr<DeliveryQueueState> queue_;
  void* user_data_ = nullptr;
  // The host callbacks, by their records' Delivered index.
  std::array<HostFunction, delivered_call_count> callbacks_{};
  // Preallocated, so retirement reaches a live queue without allocating.
  // Null without release_user_data.
  ItemPtr retired_;
  ItemPtr reserved_;
  std::size_t reserved_size_ = 0;
};

}  // namespace mln::delivery
