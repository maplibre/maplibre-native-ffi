#include <cstddef>
#include <memory>
#include <mutex>
#include <new>
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

auto destroy_item(DeliveryItem* item) noexcept -> void {
  item->~DeliveryItem();
  ::operator delete(item);
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

DeliveryQueueState::DeliveryQueueState(
  std::shared_ptr<core::Wake> wake
) noexcept
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
      // Copying the wake under the lock keeps it alive across the call, even
      // when the receiver releases the queue from inside the wake.
      wake = wake_;
    } else {
      tail_->next = raw;
    }
    tail_ = raw;
    ++count_;
  }
  if (wake != nullptr) wake->notify();
  return true;
}

auto DeliveryQueueState::drain(
  std::vector<ItemPtr>& out, std::vector<mln_delivery>& views
) -> void {
  while (true) {
    auto needed = std::size_t{};
    {
      const auto lock = std::scoped_lock{mutex_};
      needed = out.size() + count_;
      if (needed <= out.capacity() && needed <= views.capacity()) {
        for (auto* item = head_; item != nullptr;) {
          auto* next = item->next;
          item->next = nullptr;
          views.push_back(item->view);
          out.emplace_back(item);
          item = next;
        }
        head_ = nullptr;
        tail_ = nullptr;
        count_ = 0;
        return;
      }
    }
    // Reserving outside the lock keeps pushes from waiting on an allocation.
    out.reserve(needed);
    views.reserve(needed);
  }
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

auto drain_delivery_queue(mln_delivery_queue queue, mln_delivery_batch* out)
  -> mln_status {
  if (out == nullptr || *out != MLN_HANDLE_NULL) {
    core::set_thread_error("out_batch must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto state = resolve_delivery_queue(queue);
  if (state == nullptr) return core::recorded_handle_fault_status();
  // Allocating and registering the batch before the swap means a failure
  // loses no deliveries. The handle stays private until this call returns it.
  auto batch = std::make_shared<DeliveryBatchObject>();
  const auto handle = handle_table<DeliveryBatchObject>().insert(batch);
  try {
    state->drain(batch->items, batch->views);
  } catch (...) {
    static_cast<void>(handle_table<DeliveryBatchObject>().remove(handle));
    throw;
  }
  if (batch->items.empty()) {
    static_cast<void>(handle_table<DeliveryBatchObject>().remove(handle));
    return MLN_STATUS_NOT_READY;
  }
  *out = handle;
  return MLN_STATUS_OK;
}

auto get_delivery_batch(mln_delivery_batch batch, mln_delivery_batch_view* out)
  -> mln_status {
  if (out == nullptr || out->size < sizeof(mln_delivery_batch_view)) {
    core::set_thread_error(
      "out_view must not be null and must have a valid size"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto live = handle_table<DeliveryBatchObject>().lease(batch);
  if (live == nullptr) return core::recorded_handle_fault_status();
  *out = mln_delivery_batch_view{
    .size = sizeof(mln_delivery_batch_view),
    .delivery_size = sizeof(mln_delivery),
    .deliveries = live->views.data(),
    .delivery_count = live->views.size(),
  };
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
