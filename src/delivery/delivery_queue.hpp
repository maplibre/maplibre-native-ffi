#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "delivery/capture.hpp"
#include "maplibre_native_c.h"

namespace mln::core {
class Wake;
}  // namespace mln::core

namespace mln::delivery {

// A message span includes its source string's terminator in the event view.
inline auto event_message(std::string_view message) -> std::string_view {
  if (message.size() > UINT32_MAX - 1) {
    throw std::length_error{"runtime event message is too large"};
  }
  return message;
}

// Points the views of `event`'s payload at `strings`, which hold the payload's
// text in the order that its type takes it: a font stack's names for GLYPHS,
// and the ID then the URL for SPRITE. A font stack's array is `views`, which
// this fills. The event borrows both until it is delivered. A payload type
// without text leaves the event unchanged.
auto bind_event_strings(
  mln_runtime_event& event, const std::vector<std::string>& strings,
  std::vector<mln_buffer_view>& views
) -> void;

// A host function pointer of any type, which a call's invoke casts back.
using HostFunction = void (*)();

// One queued call and the copy of its arguments, in a single allocation: this
// header, then the arena storage that the copy's pointers reach. Dispatch
// passes the copy to callback through invoke, then passes user_data to
// release. Either may be null.
struct DeliveryItem {
  DeliveryItem* next = nullptr;
  void* user_data = nullptr;
  // The copied argument record, which invoke casts back.
  void* arguments = nullptr;
  // Disposes the handles that the copy owns, unless the call received them.
  // Null for a call that owns nothing.
  void (*discard)(DeliveryItem& item) noexcept = nullptr;
  bool taken = false;
  // Event spans own vectors and a message arena instead of an inline copy.
  void (*destroy)(DeliveryItem*) noexcept = nullptr;
  void (*invoke)(DeliveryItem& item) noexcept = nullptr;
  HostFunction callback = nullptr;
  mln_user_data_release release = nullptr;
};

static_assert(std::is_trivially_destructible_v<DeliveryItem>);

// Passes one copied argument record to the host callback of a call.
template <class Record>
auto invoke_delivered(DeliveryItem& item) noexcept -> void {
  Delivered<Record>::invoke(
    reinterpret_cast<typename Delivered<Record>::Callback>(item.callback),
    item.user_data, *static_cast<const Record*>(item.arguments)
  );
}

// Destroying an item discards what the host did not take.
struct ItemDeleter {
  auto operator()(DeliveryItem* item) const noexcept -> void;
};

using ItemPtr = std::unique_ptr<DeliveryItem, ItemDeleter>;

// Releases an item without disposing what it owns, as process exit requires.
auto forget_item(ItemPtr item) noexcept -> void;

// A call whose byte views borrow storage that `owner` keeps alive, such as a
// readback's image, instead of copying it. Its arena starts after it.
struct OwningDeliveryItem final : DeliveryItem {
  std::shared_ptr<const void> owner;
};

namespace detail {

auto allocate_item(std::size_t size) -> DeliveryItem*;
auto allocate_owning_item(std::size_t size, std::shared_ptr<const void> owner)
  -> DeliveryItem*;

}  // namespace detail

// Copies an argument record into one allocation, the record first in the
// arena. The copy runs twice: once to measure on an Arena<false>, once to
// write on an Arena<true>. A copy that cannot be made throws, leaving nothing
// allocated.
template <class Record>
auto make_item(void* user_data, const Record& source) -> ItemPtr {
  auto measure = Arena<false>{nullptr, sizeof(DeliveryItem)};
  static_cast<void>(measure.template allocate<Record>(1));
  static_cast<void>(capture_element(measure, source));
  auto item = ItemPtr{detail::allocate_item(measure.offset)};
  item->user_data = user_data;
  auto write = Arena<true>{
    reinterpret_cast<std::byte*>(item.get()), sizeof(DeliveryItem),
    measure.offset
  };
  auto* record = write.template allocate<Record>(1);
  *record = capture_element(write, source);
  item->arguments = record;
  return item;
}

// Copies and disposes the values of one completion result, for a completion
// that delivers through a queue.
struct CompletionCapture {
  void (*measure)(Arena<false>& arena, const void* values, std::size_t count);
  auto (*write)(Arena<true>& arena, const void* values, std::size_t count)
    -> const void*;
};

template <class T>
auto measure_values(Arena<false>& arena, const void* values, std::size_t count)
  -> void {
  static_cast<void>(span(
    arena, static_cast<const T*>(values), count,
    [](auto& storage, const T& value) {
      return capture_element(storage, value);
    }
  ));
}

template <class T>
auto write_values(Arena<true>& arena, const void* values, std::size_t count)
  -> const void* {
  return span(
    arena, static_cast<const T*>(values), count,
    [](auto& storage, const T& value) {
      return capture_element(storage, value);
    }
  );
}

// A completion value owns no handle, so a dropped completion leaks nothing; a
// function returns a new handle through an out parameter instead.
template <class T>
  requires(!element_owns_handles(static_cast<const T*>(nullptr)))
inline constexpr auto completion_capture = CompletionCapture{
  .measure = &measure_values<T>,
  .write = &write_values<T>,
};

// A call that only releases user_data.
auto make_release_call(void* user_data, mln_user_data_release release)
  -> ItemPtr;

enum class DispatchResult : std::uint8_t { Ran, Empty, Busy };

// The queue behind one mln_delivery_queue handle. Registrations hold it, so a
// released queue stays alive, closed, until the last of them retires.
//
// Calls run in push order, and the queue wakes the receiver when it goes from
// empty to nonempty.
class DeliveryQueueState final {
 public:
  explicit DeliveryQueueState(std::shared_ptr<core::Wake> wake);
  DeliveryQueueState(const DeliveryQueueState&) = delete;
  DeliveryQueueState(DeliveryQueueState&&) = delete;
  auto operator=(const DeliveryQueueState&) -> DeliveryQueueState& = delete;
  auto operator=(DeliveryQueueState&&) -> DeliveryQueueState& = delete;
  ~DeliveryQueueState();

  // Appends a call and wakes the receiver when the queue was empty. Returns
  // false and leaves the item with the caller when the queue is closed.
  // During process exit the item is dropped without disposal and this reports
  // true.
  [[nodiscard]] auto push(ItemPtr& item) noexcept -> bool;

  // Appends to a tail event span of the same registration, or starts one
  // after another call. Events share contiguous record and message storage,
  // without a capture or allocation for each event. A failed append leaves
  // the queue unchanged.
  auto push_event(
    mln_runtime_event event, std::string_view message, void* user_data,
    mln_runtime_event_callback callback, std::vector<std::string> strings = {}
  ) -> void;

  // Runs every queued call in push order on the calling thread. A call that
  // closes the queue abandons the rest. While one dispatch runs, another
  // returns Busy, and the running one also runs what was queued by then.
  auto dispatch() noexcept -> DispatchResult;

  // Refuses later pushes, stops the wake, and discards queued calls.
  auto close() noexcept -> void;

 private:
  // Runs the calls queued now. Returns whether it ran any.
  auto run_calls() noexcept -> bool;

  std::mutex mutex_;
  DeliveryItem* head_ = nullptr;
  DeliveryItem* tail_ = nullptr;
  bool closed_ = false;
  // Set while a dispatch runs calls. A dispatch that finds one running sets
  // dispatch_requested_, which makes the running one take the calls again.
  std::atomic_bool dispatching_ = false;
  std::atomic_bool dispatch_requested_ = false;
  std::shared_ptr<core::Wake> wake_;
};

// Resolves a queue handle, recording the thread error and the status that
// base.h assigns to an invalid or released handle.
auto resolve_delivery_queue(mln_delivery_queue queue)
  -> std::shared_ptr<DeliveryQueueState>;

auto create_delivery_queue(
  const mln_delivery_queue_options* options, mln_delivery_queue* out_queue
) -> mln_status;
auto release_delivery_queue(mln_delivery_queue queue) noexcept -> void;
auto dispatch_delivery_queue(mln_delivery_queue queue) -> mln_status;

}  // namespace mln::delivery
