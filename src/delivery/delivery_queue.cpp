#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "delivery/delivery_queue.hpp"

#include "diagnostics/diagnostics.hpp"
#include "execution/process_exit.hpp"
#include "handles/handle_table.hpp"

namespace mln::delivery {
namespace {

struct DeliveryQueueObject {
  std::shared_ptr<DeliveryQueueState> state;
};

struct DeliveryBatchObject {
  std::mutex mutex;
  std::vector<ItemPtr> items;
  // Contiguous copies of each item's view, which point into the items.
  std::vector<mln_delivery> views;
};

struct EventDeliveryItem final : DeliveryItem {
  std::vector<mln_runtime_event> events;
  std::string messages;

  mln_event_batch_view batch_view{};

  explicit EventDeliveryItem(void* user_data) {
    view.type = MLN_DELIVERY_TYPE_RUNTIME_EVENTS;
    view.user_data = user_data;
    view.payload.runtime_events.events = &batch_view;
    destroy = [](DeliveryItem* item) noexcept {
      delete static_cast<EventDeliveryItem*>(item);
    };
  }

  auto append(mln_runtime_event event, std::string_view message) -> void {
    message = event_message(message);
    event.message = mln_buffer_view{.data = nullptr, .size = message.size()};
    // Reserve both before changing their sizes. A failed reserve can move
    // storage, so refresh the event span on either exit.
    try {
      if (events.size() == events.capacity()) {
        events.reserve(std::max(std::size_t{16}, events.size() * 2));
      }
      if (!message.empty()) {
        messages.reserve(messages.size() + message.size() + 1);
        messages.append(message);
        messages.push_back('\0');
      }
      events.push_back(event);
    } catch (...) {
      refresh();
      throw;
    }
    refresh();
  }

  // Finalize pointers after the item leaves the queue: appends can move the
  // shared message storage while the item is still queued.
  auto finalize() noexcept -> void {
    auto offset = std::size_t{0};
    for (auto& event : events) {
      if (event.message.size == 0) continue;
      event.message.data = messages.data() + offset;
      offset += event.message.size + 1;
    }
  }

  auto refresh() noexcept -> void {
    batch_view = mln_event_batch_view{
      .size = sizeof(mln_event_batch_view),
      .event_size = sizeof(mln_runtime_event),
      .events = events.data(),
      .event_count = events.size(),
    };
  }
};

auto destroy_item(DeliveryItem* item) noexcept -> void {
  if (item->destroy != nullptr) {
    item->destroy(item);
  } else {
    item->~DeliveryItem();
    ::operator delete(item);
  }
}

}  // namespace
}  // namespace mln::delivery

namespace mln::core {

template <>
struct HandleTraits<mln::delivery::DeliveryQueueObject> {
  static constexpr auto kind = HandleKind::DeliveryQueue;
  static constexpr auto leasable = true;
};

template <>
struct HandleTraits<mln::delivery::DeliveryBatchObject> {
  static constexpr auto kind = HandleKind::DeliveryBatch;
  static constexpr auto leasable = true;
};

}  // namespace mln::core

namespace mln::delivery {

using core::handle_table;

auto ItemDeleter::operator()(DeliveryItem* item) const noexcept -> void {
  if (item == nullptr) return;
  if (!item->taken && item->discard != nullptr) item->discard(*item);
  destroy_item(item);
}

auto forget_item(ItemPtr item) noexcept -> void {
  if (item == nullptr) return;
  destroy_item(item.release());
}

namespace detail {

auto allocate_item(std::size_t size) -> DeliveryItem* {
  auto* storage = ::operator new(size);
  return new (storage) DeliveryItem{};
}

}  // namespace detail

auto make_light_item(mln_delivery_type type, void* user_data) -> ItemPtr {
  auto item = ItemPtr{detail::allocate_item(sizeof(DeliveryItem))};
  item->view.type = type;
  item->view.user_data = user_data;
  return item;
}

DeliveryQueueState::DeliveryQueueState(std::shared_ptr<core::Wake> wake)
    : wake_(std::move(wake)) {}

DeliveryQueueState::~DeliveryQueueState() { close(); }

auto DeliveryQueueState::push(ItemPtr& item) noexcept -> bool {
  if (core::process_exiting()) {
    forget_item(std::move(item));
    return true;
  }
  auto wake = std::shared_ptr<core::Wake>{};
  {
    const auto lock = std::scoped_lock{mutex_};
    if (closed_) return false;
    auto* raw = item.release();
    raw->next = nullptr;
    if (tail_ == nullptr) {
      head_ = raw;
    } else {
      tail_->next = raw;
    }
    if (tail_ == nullptr || wake_on_push_) {
      wake_on_push_ = false;
      // Copying the wake under the lock keeps it alive across the call, even
      // when the receiver releases the queue from inside the wake.
      wake = wake_;
    }
    tail_ = raw;
    ++count_;
  }
  if (wake != nullptr) wake->notify();
  return true;
}

auto DeliveryQueueState::push_event(
  mln_runtime_event event, std::string_view message, void* user_data
) -> void {
  if (core::process_exiting()) return;
  auto wake = std::shared_ptr<core::Wake>{};
  {
    const auto lock = std::scoped_lock{mutex_};
    if (closed_) return;
    if (
      tail_ != nullptr &&
      tail_->view.type == MLN_DELIVERY_TYPE_RUNTIME_EVENTS &&
      tail_->view.user_data == user_data
    ) {
      static_cast<EventDeliveryItem*>(tail_)->append(event, message);
    } else {
      auto item = std::make_unique<EventDeliveryItem>(user_data);
      item->append(event, message);
      auto* raw = item.release();
      if (tail_ == nullptr) {
        head_ = raw;
        wake = wake_;
      } else {
        tail_->next = raw;
      }
      tail_ = raw;
      ++count_;
    }
    if (wake_on_push_) {
      wake_on_push_ = false;
      wake = wake_;
    }
  }
  if (wake != nullptr) wake->notify();
}

auto DeliveryQueueState::drain(mln_delivery_batch_view& view)
  -> mln_delivery_batch {
  const auto drain_lock = std::scoped_lock{drain_mutex_};
  auto* items = static_cast<DeliveryItem*>(nullptr);
  auto* last = static_cast<DeliveryItem*>(nullptr);
  auto count = std::size_t{};
  {
    const auto lock = std::scoped_lock{mutex_};
    items = std::exchange(head_, nullptr);
    last = std::exchange(tail_, nullptr);
    count = std::exchange(count_, 0);
  }
  if (items == nullptr) return MLN_HANDLE_NULL;
  auto batch = std::shared_ptr<DeliveryBatchObject>{};
  auto handle = MLN_HANDLE_NULL;
  try {
    batch = std::make_shared<DeliveryBatchObject>();
    batch->items.reserve(count);
    batch->views.reserve(count);
    handle = handle_table<DeliveryBatchObject>().insert(batch);
  } catch (...) {
    {
      const auto lock = std::scoped_lock{mutex_};
      if (!closed_) {
        // Pushes only append and drains are serialized, so the items go back
        // ahead of everything pushed since.
        last->next = head_;
        head_ = std::exchange(items, nullptr);
        if (tail_ == nullptr) tail_ = last;
        count_ += count;
        wake_on_push_ = true;
      }
    }
    // Closure may have finished while the batch allocation was in progress.
    // Dispose outside the lock, as close() does for pending items.
    while (items != nullptr) {
      auto* next = items->next;
      if (core::process_exiting()) {
        forget_item(ItemPtr{items});
      } else {
        static_cast<void>(ItemPtr{items});
      }
      items = next;
    }
    throw;
  }
  while (items != nullptr) {
    auto* next = std::exchange(items->next, nullptr);
    if (items->view.type == MLN_DELIVERY_TYPE_RUNTIME_EVENTS)
      static_cast<EventDeliveryItem*>(items)->finalize();
    batch->views.push_back(items->view);
    batch->items.emplace_back(items);
    items = next;
  }
  view = mln_delivery_batch_view{
    .size = sizeof(mln_delivery_batch_view),
    .delivery_size = sizeof(mln_delivery),
    .deliveries = batch->views.data(),
    .delivery_count = batch->views.size(),
  };
  return handle;
}

auto DeliveryQueueState::close() noexcept -> void {
  auto* pending = static_cast<DeliveryItem*>(nullptr);
  auto wake = std::shared_ptr<core::Wake>{};
  {
    const auto lock = std::scoped_lock{mutex_};
    closed_ = true;
    pending = head_;
    head_ = nullptr;
    tail_ = nullptr;
    count_ = 0;
    wake = std::move(wake_);
  }
  if (wake != nullptr) wake->close();
  // The wake's user data is released when its last reference drops: here, or
  // in a push whose notify is still running.
  wake.reset();
  while (pending != nullptr) {
    auto* next = pending->next;
    if (core::process_exiting()) {
      forget_item(ItemPtr{pending});
    } else {
      static_cast<void>(ItemPtr{pending});
    }
    pending = next;
  }
}

auto resolve_delivery_queue(mln_delivery_queue queue)
  -> std::shared_ptr<DeliveryQueueState> {
  const auto live = handle_table<DeliveryQueueObject>().lease(queue);
  if (live == nullptr) return nullptr;
  return live->state;
}

auto create_delivery_queue(
  const mln_delivery_queue_options* options, mln_delivery_queue* out_queue
) -> mln_status {
  if (options == nullptr) {
    core::set_thread_error("options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_delivery_queue_options)) {
    core::set_thread_error("mln_delivery_queue_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto wake_status = core::validate_wake(&options->wake);
  if (wake_status != MLN_STATUS_OK) return wake_status;
  if (out_queue == nullptr || *out_queue != MLN_HANDLE_NULL) {
    core::set_thread_error("out_queue must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  core::watch_process_exit();
  auto wake = std::shared_ptr<core::Wake>{};
  try {
    wake = std::make_shared<core::Wake>(options->wake);
    auto object = std::make_shared<DeliveryQueueObject>();
    object->state = std::make_shared<DeliveryQueueState>(wake);
    *out_queue = handle_table<DeliveryQueueObject>().insert(std::move(object));
  } catch (...) {
    // A rejected wake never releases the caller's user data.
    if (wake != nullptr) wake->reject();
    throw;
  }
  wake->accept();
  return MLN_STATUS_OK;
}

auto release_delivery_queue(mln_delivery_queue queue) noexcept -> void {
  if (queue == MLN_HANDLE_NULL) return;
  auto object = std::shared_ptr<DeliveryQueueObject>{};
  try {
    object = handle_table<DeliveryQueueObject>().remove(queue);
  } catch (...) {
    return;
  }
  if (object != nullptr) object->state->close();
}

auto drain_delivery_queue(
  mln_delivery_queue queue, mln_delivery_batch* out_batch,
  mln_delivery_batch_view* out_view
) -> mln_status {
  if (out_batch == nullptr || *out_batch != MLN_HANDLE_NULL) {
    core::set_thread_error("out_batch must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_view == nullptr || out_view->size < sizeof(mln_delivery_batch_view)) {
    core::set_thread_error(
      "out_view must not be null and must have a valid size"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto state = resolve_delivery_queue(queue);
  if (state == nullptr) return core::recorded_handle_fault_status();
  auto view = mln_delivery_batch_view{};
  const auto batch = state->drain(view);
  if (batch == MLN_HANDLE_NULL) return MLN_STATUS_NOT_READY;
  *out_batch = batch;
  *out_view = view;
  return MLN_STATUS_OK;
}

auto take_delivery(mln_delivery_batch batch, std::size_t index) -> mln_status {
  const auto live = handle_table<DeliveryBatchObject>().lease(batch);
  if (live == nullptr) return core::recorded_handle_fault_status();
  const auto lock = std::scoped_lock{live->mutex};
  if (index >= live->items.size()) {
    core::set_thread_error("delivery index is out of range");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto& item = *live->items[index];
  if (item.discard == nullptr) return MLN_STATUS_OK;
  if (item.taken) {
    core::set_thread_error("delivery was already taken");
    return MLN_STATUS_INVALID_STATE;
  }
  item.taken = true;
  return MLN_STATUS_OK;
}

auto release_delivery_batch(mln_delivery_batch batch) noexcept -> void {
  if (batch == MLN_HANDLE_NULL) return;
  try {
    static_cast<void>(handle_table<DeliveryBatchObject>().remove(batch));
  } catch (...) {
  }
}

}  // namespace mln::delivery
