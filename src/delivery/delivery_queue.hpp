#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <type_traits>
#include <vector>

#include "delivery/capture.hpp"
#include "maplibre_native_c.h"
#include "wake/wake.hpp"

namespace mln::delivery {

// One queued delivery and the copy it owns, in a single allocation: this
// header, then the arena storage that the view's pointers reach.
struct DeliveryItem {
  DeliveryItem* next = nullptr;
  mln_delivery view{};
  // Disposes the handles that the copy owns, unless the host took them.
  // Null for a delivery that owns nothing.
  void (*discard)(DeliveryItem& item) noexcept = nullptr;
  bool taken = false;
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
  void (*discard)(const void* values, std::size_t count) noexcept;
  // Disposes the values a queued completion owns, or null when its values own
  // no handle.
  void (*discard_item)(DeliveryItem& item) noexcept;
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

template <class T>
auto discard_values(const void* values, std::size_t count) noexcept -> void {
  if (values == nullptr) return;
  for (std::size_t index = 0; index < count; ++index) {
    discard_element(static_cast<const T*>(values)[index]);
  }
}

template <class T>
auto discard_completion_item(DeliveryItem& item) noexcept -> void {
  const auto& result = item.view.payload.completion;
  if (result.status == MLN_STATUS_OK) {
    discard_values<T>(result.value, result.value_count);
  }
}

template <class T>
inline constexpr auto completion_capture = CompletionCapture{
  .measure = &measure_values<T>,
  .write = &write_values<T>,
  .discard = &discard_values<T>,
  .discard_item = element_owns_handles(static_cast<const T*>(nullptr))
                    ? &discard_completion_item<T>
                    : nullptr,
};

// A delivery with no copied payload, such as MLN_DELIVERY_TYPE_RETIRED or a
// completion's preallocated fallback.
auto make_light_item(mln_delivery_type type, void* user_data) -> ItemPtr;

// The queue behind one mln_delivery_queue handle. Registrations hold it, so a
// released queue stays alive, closed, until the last of them retires.
class DeliveryQueueState final {
 public:
  explicit DeliveryQueueState(std::shared_ptr<core::Wake> wake) noexcept;
  DeliveryQueueState(const DeliveryQueueState&) = delete;
  DeliveryQueueState(DeliveryQueueState&&) = delete;
  auto operator=(const DeliveryQueueState&) -> DeliveryQueueState& = delete;
  auto operator=(DeliveryQueueState&&) -> DeliveryQueueState& = delete;
  ~DeliveryQueueState();

  // Appends an item and wakes the receiver when the queue was empty. Returns
  // false and leaves the item with the caller when the queue is closed.
  // During process exit the item is dropped without disposal and this
  // reports true.
  [[nodiscard]] auto push(ItemPtr& item) noexcept -> bool;
  // Appends like push() but leaves the wake to the caller: needs_wake reports
  // whether the queue was empty, and the caller calls rewake() later from a
  // context where host code may run.
  [[nodiscard]] auto push_without_wake(ItemPtr& item, bool& needs_wake) noexcept
    -> bool;

  // Moves every queued item into out, in push order, after reserving room in
  // out and views. Allocation failure throws and leaves the items queued.
  auto drain(std::vector<ItemPtr>& out, std::vector<mln_delivery>& views)
    -> void;
  // Whether nothing is queued, so a drain can return without allocating.
  [[nodiscard]] auto empty() noexcept -> bool;
  // Wakes the receiver when deliveries are queued. Pushes wake only on the
  // empty-to-nonempty transition, so a drain that fails after that wake calls
  // this to keep them from being stranded.
  auto rewake() noexcept -> void;

  // Refuses later pushes, stops the wake, and discards queued items.
  auto close() noexcept -> void;

 private:
  std::mutex mutex_;
  DeliveryItem* head_ = nullptr;
  DeliveryItem* tail_ = nullptr;
  std::size_t count_ = 0;
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
auto drain_delivery_queue(mln_delivery_queue queue, mln_delivery_batch* out)
  -> mln_status;
auto get_delivery_batch(mln_delivery_batch batch, mln_delivery_batch_view* out)
  -> mln_status;
auto take_delivery(mln_delivery_batch batch, std::size_t index) -> mln_status;
auto release_delivery_batch(mln_delivery_batch batch) noexcept -> void;

}  // namespace mln::delivery
