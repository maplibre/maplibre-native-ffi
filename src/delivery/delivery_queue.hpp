#pragma once

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
#include "wake/wake.hpp"

namespace mln::delivery {

// A message span includes its source string's terminator in the batch view.
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

// One queued delivery and the copy it owns, in a single allocation: this
// header, then the arena storage that the view's pointers reach.
struct DeliveryItem {
  DeliveryItem* next = nullptr;
  mln_delivery view{};
  // Disposes the handles that the copy owns, unless the host took them.
  // Null for a delivery that owns nothing.
  void (*discard)(DeliveryItem& item) noexcept = nullptr;
  bool taken = false;
  // Event spans own vectors and a message arena instead of an inline copy.
  void (*destroy)(DeliveryItem*) noexcept = nullptr;
};

static_assert(std::is_trivially_destructible_v<DeliveryItem>);

// Destroying an item discards what the host did not take.
struct ItemDeleter {
  auto operator()(DeliveryItem* item) const noexcept -> void;
};

using ItemPtr = std::unique_ptr<DeliveryItem, ItemDeleter>;

// Releases an item without disposing what it owns, as process exit requires.
auto forget_item(ItemPtr item) noexcept -> void;

namespace detail {

auto allocate_item(std::size_t size) -> DeliveryItem*;

}  // namespace detail

// Copies a payload into one allocation. Fill writes the payload into the
// delivery's payload member and runs twice: once to measure on an
// Arena<false>, once to write on an Arena<true>. A copy that cannot be made
// throws, leaving nothing allocated.
template <class Fill>
auto make_item(mln_delivery_type type, void* user_data, Fill fill) -> ItemPtr {
  auto measure = Arena<false>{nullptr, sizeof(DeliveryItem)};
  auto scratch = mln_delivery_payload{};
  fill(measure, scratch);
  auto item = ItemPtr{detail::allocate_item(measure.offset)};
  item->view.type = type;
  item->view.user_data = user_data;
  auto write = Arena<true>{
    reinterpret_cast<std::byte*>(item.get()), sizeof(DeliveryItem),
    measure.offset
  };
  fill(write, item->view.payload);
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

// A delivery with no copied payload, such as MLN_DELIVERY_TYPE_RETIRED.
auto make_light_item(mln_delivery_type type, void* user_data) -> ItemPtr;

// The queue behind one mln_delivery_queue handle. Registrations hold it, so a
// released queue stays alive, closed, until the last of them retires.
class DeliveryQueueState final {
 public:
  explicit DeliveryQueueState(std::shared_ptr<core::Wake> wake);
  DeliveryQueueState(const DeliveryQueueState&) = delete;
  DeliveryQueueState(DeliveryQueueState&&) = delete;
  auto operator=(const DeliveryQueueState&) -> DeliveryQueueState& = delete;
  auto operator=(DeliveryQueueState&&) -> DeliveryQueueState& = delete;
  ~DeliveryQueueState();

  // Appends an item and wakes the receiver when the queue was empty or the
  // last drain failed. Returns false and leaves the item with the caller when
  // the queue is closed. During process exit the item is dropped without
  // disposal and this reports true.
  [[nodiscard]] auto push(ItemPtr& item) noexcept -> bool;

  // Appends to a tail event span, or starts one after another delivery kind.
  // Events share contiguous record and message storage, without a capture or
  // allocation for each event. A failed append leaves the queue unchanged.
  auto push_event(
    mln_runtime_event event, std::string_view message, void* user_data,
    std::vector<std::string> strings = {}
  ) -> void;

  // Moves every queued item, in push order, into a new owned batch, writes
  // its view, and returns its handle; or returns the null handle when nothing
  // is queued. Allocation failure throws and returns every item to an open
  // queue in order, making the next push wake the receiver. A closed queue
  // discards the items.
  auto drain(mln_delivery_batch_view& view) -> mln_delivery_batch;

  // Refuses later pushes, stops the wake, and discards queued items.
  auto close() noexcept -> void;

 private:
  // Serializes drains, so a failed drain returns its items to the head of
  // the queue ahead of any later push.
  std::mutex drain_mutex_;
  std::mutex mutex_;
  DeliveryItem* head_ = nullptr;
  DeliveryItem* tail_ = nullptr;
  std::size_t count_ = 0;
  bool wake_on_push_ = false;
  bool closed_ = false;
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
auto drain_delivery_queue(
  mln_delivery_queue queue, mln_delivery_batch* out_batch,
  mln_delivery_batch_view* out_view
) -> mln_status;
auto take_delivery(mln_delivery_batch batch, std::size_t index) -> mln_status;
auto release_delivery_batch(mln_delivery_batch batch) noexcept -> void;

}  // namespace mln::delivery
