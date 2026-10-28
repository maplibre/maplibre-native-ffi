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
#include "wake/wake.hpp"

namespace mln::delivery {
namespace {

auto string_view_of(const std::string& text) noexcept -> mln_buffer_view {
  return mln_buffer_view{.data = text.c_str(), .size = text.size()};
}

}  // namespace

auto bind_event_strings(
  mln_runtime_event& event, const std::vector<std::string>& strings,
  std::vector<mln_buffer_view>& views
) -> void {
  switch (event.payload_type) {
    case MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS:
      views.clear();
      views.reserve(strings.size());
      for (const auto& name : strings) views.push_back(string_view_of(name));
      event.payload.glyphs.font_stack = views.empty() ? nullptr : views.data();
      event.payload.glyphs.font_count = views.size();
      return;
    case MLN_RUNTIME_EVENT_PAYLOAD_SPRITE:
      if (strings.size() != 2) {
        throw std::logic_error{"a sprite payload takes an ID and a URL"};
      }
      event.payload.sprite.id = string_view_of(strings[0]);
      event.payload.sprite.url = string_view_of(strings[1]);
      return;
    default:
      return;
  }
}

namespace {

struct DeliveryQueueObject {
  std::shared_ptr<DeliveryQueueState> state;
};

// The payload text of one event, and the font stack array that points at it.
// The event's views point into the vectors' heap storage, which stays put when
// the vectors move.
struct EventStrings {
  std::vector<std::string> strings;
  std::vector<mln_buffer_view> views;
};

struct EventDeliveryItem final : DeliveryItem {
  std::vector<mln_runtime_event> events;
  std::string messages;
  // Only the events whose payload carries text have an entry.
  std::vector<EventStrings> strings;

  mln_event_batch_view batch_view{};
  RuntimeEventArguments record{.events = &batch_view};

  EventDeliveryItem(void* user_data, mln_runtime_event_callback function) {
    this->user_data = user_data;
    arguments = &record;
    callback = reinterpret_cast<HostFunction>(function);
    invoke = &run;
    destroy = [](DeliveryItem* item) noexcept {
      delete static_cast<EventDeliveryItem*>(item);
    };
  }

  // Appends can move the shared message storage while the item is queued, so
  // the call finalizes the event pointers.
  static auto run(DeliveryItem& item) noexcept -> void {
    static_cast<EventDeliveryItem&>(item).finalize();
    invoke_delivered<RuntimeEventArguments>(item);
  }

  auto append(
    mln_runtime_event event, std::string_view message,
    std::vector<std::string> payload_strings
  ) -> void {
    message = event_message(message);
    event.message = mln_buffer_view{.data = nullptr, .size = message.size()};
    auto text =
      EventStrings{.strings = std::move(payload_strings), .views = {}};
    bind_event_strings(event, text.strings, text.views);
    // Reserve every vector before changing any size. A failed reserve can move
    // storage, so refresh the event span on either exit.
    try {
      if (events.size() == events.capacity()) {
        events.reserve(std::max(std::size_t{16}, events.size() * 2));
      }
      if (!text.strings.empty()) strings.reserve(strings.size() + 1);
      if (!message.empty()) {
        messages.reserve(messages.size() + message.size() + 1);
        messages.append(message);
        messages.push_back('\0');
      }
      if (!text.strings.empty()) strings.push_back(std::move(text));
      events.push_back(event);
    } catch (...) {
      refresh();
      throw;
    }
    refresh();
  }

  auto finalize() noexcept -> void {
    auto offset = std::size_t{0};
    for (auto& event : events) {
      if (event.message.size == 0) {
        event.message.data = "";
        continue;
      }
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

auto allocate_owning_item(std::size_t size, std::shared_ptr<const void> owner)
  -> DeliveryItem* {
  auto* storage = ::operator new(size);
  auto* item = new (storage) OwningDeliveryItem{};
  item->owner = std::move(owner);
  item->destroy = [](DeliveryItem* base) noexcept {
    auto* owning = static_cast<OwningDeliveryItem*>(base);
    owning->~OwningDeliveryItem();
    ::operator delete(owning);
  };
  return item;
}

}  // namespace detail

auto make_release_call(void* user_data, mln_user_data_release release)
  -> ItemPtr {
  auto item = ItemPtr{detail::allocate_item(sizeof(DeliveryItem))};
  item->user_data = user_data;
  item->release = release;
  return item;
}

namespace {

auto unlink_held(HeldUserData*& head, HeldUserData& node) noexcept -> void {
  if (node.previous != nullptr) {
    node.previous->next = node.next;
  } else {
    head = node.next;
  }
  if (node.next != nullptr) node.next->previous = node.previous;
}

auto report_abandoned(mln_user_data_release abandon, void* user_data) noexcept
  -> void {
  try {
    abandon(user_data);
  } catch (...) {
    // Host release callbacks must not unwind through the C boundary.
  }
}

// Destroys items that will never be delivered, disposing what they own
// unless the process is exiting. A non-null abandon receives the user_data of
// each last call after its item is gone.
auto abandon_items(
  DeliveryItem* items, mln_user_data_release abandon = nullptr
) noexcept -> void {
  while (items != nullptr) {
    auto* next = items->next;
    auto* const user_data = items->user_data;
    const auto last_call = items->last_call;
    if (core::process_exiting()) {
      forget_item(ItemPtr{items});
    } else {
      static_cast<void>(ItemPtr{items});
      if (last_call && abandon != nullptr) {
        report_abandoned(abandon, user_data);
      }
    }
    items = next;
  }
}

auto run_call(DeliveryItem& item) noexcept -> void {
  // The call receives the handles that the copy owns.
  item.taken = true;
  try {
    if (item.invoke != nullptr) item.invoke(item);
  } catch (...) {
    // Host callbacks must not unwind through the C boundary.
  }
  try {
    if (item.release != nullptr) item.release(item.user_data);
  } catch (...) {
    // Host release callbacks must not unwind through the C boundary.
  }
}

}  // namespace

AbandonHold::AbandonHold(AbandonHold&& other) noexcept
    : queue_(std::move(other.queue_)),
      node_(std::exchange(other.node_, nullptr)) {}

auto AbandonHold::operator=(AbandonHold&& other) noexcept -> AbandonHold& {
  if (this != &other) {
    reset();
    queue_ = std::move(other.queue_);
    node_ = std::exchange(other.node_, nullptr);
  }
  return *this;
}

AbandonHold::~AbandonHold() { reset(); }

auto AbandonHold::reset() noexcept -> void {
  if (node_ != nullptr) queue_->withdraw(std::exchange(node_, nullptr));
  queue_.reset();
}

auto hold_user_data(
  const std::shared_ptr<DeliveryQueueState>& queue, void* user_data
) -> AbandonHold {
  auto hold = AbandonHold{};
  if (queue == nullptr || queue->abandon_ == nullptr) return hold;
  auto node = std::make_unique<HeldUserData>();
  node->user_data = user_data;
  {
    const auto lock = std::scoped_lock{queue->mutex_};
    if (queue->closed_) return hold;
    node->next = queue->held_;
    if (queue->held_ != nullptr) queue->held_->previous = node.get();
    queue->held_ = node.get();
  }
  hold.queue_ = queue;
  hold.node_ = node.release();
  return hold;
}

DeliveryQueueState::DeliveryQueueState(
  std::shared_ptr<core::Wake> wake, mln_user_data_release abandon
)
    : abandon_(abandon), wake_(std::move(wake)) {}

DeliveryQueueState::~DeliveryQueueState() { close(); }

auto DeliveryQueueState::withdraw(HeldUserData* node) noexcept -> void {
  {
    const auto lock = std::scoped_lock{mutex_};
    // close() took and freed every node that it found.
    if (closed_) return;
    unlink_held(held_, *node);
  }
  delete node;
}

auto DeliveryQueueState::push(ItemPtr& item) noexcept -> bool {
  if (core::process_exiting()) {
    forget_item(std::move(item));
    return true;
  }
  return append(item, nullptr);
}

auto DeliveryQueueState::push(ItemPtr& item, AbandonHold& hold) noexcept
  -> bool {
  if (core::process_exiting()) {
    forget_item(std::move(item));
    hold.reset();
    return true;
  }
  auto* const node = std::exchange(hold.node_, nullptr);
  hold.queue_.reset();
  return append(item, node);
}

auto DeliveryQueueState::append(ItemPtr& item, HeldUserData* settled) noexcept
  -> bool {
  auto wake = std::shared_ptr<core::Wake>{};
  {
    const auto lock = std::scoped_lock{mutex_};
    // A closed queue already took and reported the settled node.
    if (closed_) return false;
    if (settled != nullptr) {
      unlink_held(held_, *settled);
      item->last_call = true;
    }
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
  }
  delete settled;
  if (wake != nullptr) wake->notify();
  return true;
}

auto DeliveryQueueState::push_event(
  mln_runtime_event event, std::string_view message, void* user_data,
  mln_runtime_event_callback callback, std::vector<std::string> strings
) -> void {
  if (core::process_exiting()) return;
  auto wake = std::shared_ptr<core::Wake>{};
  {
    const auto lock = std::scoped_lock{mutex_};
    if (closed_) return;
    if (
      tail_ != nullptr && tail_->invoke == &EventDeliveryItem::run &&
      tail_->user_data == user_data &&
      tail_->callback == reinterpret_cast<HostFunction>(callback)
    ) {
      static_cast<EventDeliveryItem*>(tail_)->append(
        event, message, std::move(strings)
      );
    } else {
      auto item = std::make_unique<EventDeliveryItem>(user_data, callback);
      item->append(event, message, std::move(strings));
      auto* raw = item.release();
      if (tail_ == nullptr) {
        head_ = raw;
        wake = wake_;
      } else {
        tail_->next = raw;
      }
      tail_ = raw;
    }
  }
  if (wake != nullptr) wake->notify();
}

auto DeliveryQueueState::dispatch() noexcept -> DispatchResult {
  // A dispatch that finds another running leaves this request for it. The
  // running one checks for requests again after it stops, so none is lost.
  dispatch_requested_.store(true, std::memory_order_release);
  auto ran = false;
  auto acquired = false;
  while (!dispatching_.exchange(true, std::memory_order_acq_rel)) {
    acquired = true;
    while (dispatch_requested_.exchange(false, std::memory_order_acq_rel)) {
      if (run_calls()) ran = true;
    }
    // An exchange, not a store, so a BUSY dispatch that saw this one running
    // has its request visible to the check below.
    dispatching_.exchange(false, std::memory_order_acq_rel);
    if (!dispatch_requested_.load(std::memory_order_acquire)) break;
  }
  if (ran) return DispatchResult::Ran;
  return acquired ? DispatchResult::Empty : DispatchResult::Busy;
}

auto DeliveryQueueState::run_calls() noexcept -> bool {
  if (core::process_exiting()) return false;
  {
    const auto lock = std::scoped_lock{mutex_};
    if (closed_) return false;
    running_ = std::exchange(head_, nullptr);
    tail_ = nullptr;
  }
  auto ran = false;
  auto* exiting = static_cast<DeliveryItem*>(nullptr);
  while (true) {
    auto call = ItemPtr{};
    {
      // A call may release the queue, which takes the rest.
      const auto lock = std::scoped_lock{mutex_};
      if (closed_ || running_ == nullptr) break;
      if (core::process_exiting()) {
        exiting = std::exchange(running_, nullptr);
        break;
      }
      call = ItemPtr{running_};
      running_ = std::exchange(call->next, nullptr);
    }
    run_call(*call);
    ran = true;
  }
  abandon_items(exiting);
  return ran;
}

auto DeliveryQueueState::close() noexcept -> void {
  auto running = static_cast<DeliveryItem*>(nullptr);
  auto calls = static_cast<DeliveryItem*>(nullptr);
  auto* held = static_cast<HeldUserData*>(nullptr);
  auto wake = std::shared_ptr<core::Wake>{};
  {
    const auto lock = std::scoped_lock{mutex_};
    closed_ = true;
    running = std::exchange(running_, nullptr);
    calls = std::exchange(head_, nullptr);
    tail_ = nullptr;
    held = std::exchange(held_, nullptr);
    wake = std::move(wake_);
  }
  if (wake != nullptr) wake->close();
  // The wake's user data is released when its last reference drops: here, or
  // in a push whose notify is still running.
  wake.reset();
  const auto abandon = core::process_exiting() ? nullptr : abandon_;
  abandon_items(running, abandon);
  abandon_items(calls, abandon);
  while (held != nullptr) {
    const auto node = std::unique_ptr<HeldUserData>{held};
    held = node->next;
    if (abandon != nullptr) report_abandoned(abandon, node->user_data);
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
  if (options->wake.queue != MLN_HANDLE_NULL) {
    core::set_thread_error("a delivery queue's own wake must not name a queue");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto wake_queue = std::shared_ptr<DeliveryQueueState>{};
  const auto wake_status = core::resolve_wake(&options->wake, wake_queue);
  if (wake_status != MLN_STATUS_OK) return wake_status;
  if (out_queue == nullptr || *out_queue != MLN_HANDLE_NULL) {
    core::set_thread_error("out_queue must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  core::watch_process_exit();
  auto wake = std::shared_ptr<core::Wake>{};
  try {
    wake = std::make_shared<core::Wake>(options->wake, nullptr);
    auto object = std::make_shared<DeliveryQueueObject>();
    object->state = std::make_shared<DeliveryQueueState>(
      wake, options->release_abandoned_user_data
    );
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

auto dispatch_delivery_queue(mln_delivery_queue queue) -> mln_status {
  const auto state = resolve_delivery_queue(queue);
  if (state == nullptr) return core::recorded_handle_fault_status();
  switch (state->dispatch()) {
    case DispatchResult::Ran:
      return MLN_STATUS_OK;
    case DispatchResult::Empty:
      return MLN_STATUS_NOT_READY;
    case DispatchResult::Busy:
      core::set_thread_error("the delivery queue is already dispatching");
      return MLN_STATUS_BUSY;
  }
  return MLN_STATUS_NATIVE_ERROR;
}

}  // namespace mln::delivery
