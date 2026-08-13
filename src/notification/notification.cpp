#include <algorithm>
#include <utility>

#include "notification/notification.hpp"

#include "diagnostics/diagnostics.hpp"
#include "handles/handle_table.hpp"

namespace mln::core {

namespace {

constexpr auto valid_endpoint_kind(std::uint32_t kind) noexcept -> bool {
  return kind >= MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS &&
         kind <= MLN_NOTIFICATION_ENDPOINT_DRIVER_WORK;
}

}  // namespace

template <>
struct HandleTraits<NotificationSourceObject> {
  static constexpr auto kind = HandleKind::NotificationSource;
  static constexpr auto leasable = true;
};

template <>
struct HandleTraits<ReadyBatchObject> {
  static constexpr auto kind = HandleKind::ReadyBatch;
  static constexpr auto leasable = true;
};

NotificationEndpoint::NotificationEndpoint(
  std::shared_ptr<NotificationSourceObject> source, std::uint64_t id,
  std::uint32_t kind, bool sticky
)
    : source_(std::move(source)), id_(id) {
  if (!valid_endpoint_kind(kind)) {
    throw std::invalid_argument{"notification endpoint kind is invalid"};
  }
  if (id == MLN_HANDLE_NULL) {
    throw std::invalid_argument{"notification endpoint ID is null"};
  }
  // Association activates the endpoint before publishing this lease.
  static_cast<void>(sticky);
}

NotificationEndpoint::~NotificationEndpoint() { detach(); }

auto NotificationEndpoint::mark_ready() const noexcept -> void {
  if (active_.load(std::memory_order_acquire)) {
    source_->mark_ready(id_, this);
  }
}

auto NotificationEndpoint::clear_ready() const noexcept -> void {
  if (active_.load(std::memory_order_acquire)) {
    source_->clear_ready(id_, this);
  }
}

auto NotificationEndpoint::detach() const noexcept -> void {
  if (active_.exchange(false, std::memory_order_acq_rel)) {
    source_->detach(id_, this);
  }
}

auto NotificationEndpoint::source() const noexcept
  -> std::shared_ptr<NotificationSourceObject> {
  return source_;
}

auto NotificationSourceObject::associate(
  std::uint64_t id, std::uint32_t kind, bool sticky
) -> std::shared_ptr<NotificationEndpoint> {
  if (!valid_endpoint_kind(kind) || id == MLN_HANDLE_NULL) {
    set_thread_error("notification endpoint kind or ID is invalid");
    return nullptr;
  }
  auto endpoint = std::make_shared<NotificationEndpoint>(
    shared_from_this(), id, kind, sticky
  );
  {
    const std::scoped_lock lock(mutex_);
    if (closing_) {
      set_thread_error("notification source is closing");
      return nullptr;
    }
    if (endpoints_.contains(id)) {
      set_thread_error("notification endpoint is already associated");
      return nullptr;
    }
    endpoints_.emplace(
      id, EndpointState{
            .kind = kind,
            .sticky = sticky,
            .owner = endpoint.get(),
            .ready = false,
          }
    );
  }
  endpoint->active_.store(true, std::memory_order_release);
  return endpoint;
}

auto NotificationSourceObject::detach(
  std::uint64_t id, const NotificationEndpoint* owner
) noexcept -> void {
  const std::scoped_lock lock(mutex_);
  const auto found = endpoints_.find(id);
  if (found != endpoints_.end() && found->second.owner == owner) {
    endpoints_.erase(found);
    recompute_signaled_locked();
  }
}

auto NotificationSourceObject::prepare_callback_locked() noexcept
  -> CallbackInvocation {
  if (!callback_enabled_ || callback_ == nullptr) {
    return {};
  }
  ++callbacks_in_flight_;
  return CallbackInvocation{
    .callback = callback_, .user_data = callback_user_data_
  };
}

auto NotificationSourceObject::invoke(CallbackInvocation invocation) noexcept
  -> void {
  if (invocation.callback == nullptr) {
    return;
  }
  try {
    invocation.callback(invocation.user_data);
  } catch (...) {
    // A host callback must not unwind through the C boundary.
  }
  finish_callback();
}

auto NotificationSourceObject::finish_callback() noexcept -> void {
  {
    const std::scoped_lock lock(mutex_);
    --callbacks_in_flight_;
  }
  callback_condition_.notify_all();
}

auto NotificationSourceObject::recompute_signaled_locked() noexcept -> void {
  signaled_ =
    std::ranges::any_of(endpoints_, [](const auto& entry) noexcept -> bool {
      return entry.second.ready;
    });
}

auto NotificationSourceObject::mark_ready(
  std::uint64_t id, const NotificationEndpoint* owner
) noexcept -> void {
  auto invocation = CallbackInvocation{};
  {
    const std::scoped_lock lock(mutex_);
    const auto found = endpoints_.find(id);
    if (found == endpoints_.end() || found->second.owner != owner || closing_) {
      return;
    }
    found->second.ready = true;
    if (!signaled_) {
      signaled_ = true;
      invocation = prepare_callback_locked();
    }
  }
  invoke(invocation);
}

auto NotificationSourceObject::clear_ready(
  std::uint64_t id, const NotificationEndpoint* owner
) noexcept -> void {
  const std::scoped_lock lock(mutex_);
  const auto found = endpoints_.find(id);
  if (
    found == endpoints_.end() || found->second.owner != owner ||
    !found->second.sticky
  ) {
    return;
  }
  found->second.ready = false;
  recompute_signaled_locked();
}

auto NotificationSourceObject::set_callback(
  mln_notification_callback callback, void* user_data
) -> mln_status {
  if (callback == nullptr) {
    set_thread_error("notification callback must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto invocation = CallbackInvocation{};
  {
    auto lock = std::unique_lock{mutex_};
    if (closing_) {
      set_thread_error("notification source is closing");
      return MLN_STATUS_INVALID_STATE;
    }
    callback_enabled_ = false;
    callback_condition_.wait(lock, [this]() noexcept -> bool {
      return callbacks_in_flight_ == 0;
    });
    callback_ = callback;
    callback_user_data_ = user_data;
    callback_enabled_ = true;
    if (signaled_) {
      invocation = prepare_callback_locked();
    }
  }
  invoke(invocation);
  return MLN_STATUS_OK;
}

auto NotificationSourceObject::clear_callback() -> mln_status {
  auto lock = std::unique_lock{mutex_};
  if (closing_) {
    set_thread_error("notification source is closing");
    return MLN_STATUS_INVALID_STATE;
  }
  callback_enabled_ = false;
  callback_condition_.wait(lock, [this]() noexcept -> bool {
    return callbacks_in_flight_ == 0;
  });
  callback_ = nullptr;
  callback_user_data_ = nullptr;
  callback_enabled_ = true;
  return MLN_STATUS_OK;
}

auto NotificationSourceObject::begin_close() -> mln_status {
  auto lock = std::unique_lock{mutex_};
  if (closing_) {
    set_thread_error("notification source is already closing");
    return MLN_STATUS_INVALID_STATE;
  }
  if (!endpoints_.empty()) {
    set_thread_error("notification source still has associated endpoints");
    return MLN_STATUS_INVALID_STATE;
  }
  closing_ = true;
  callback_enabled_ = false;
  callback_condition_.wait(lock, [this]() noexcept -> bool {
    return callbacks_in_flight_ == 0;
  });
  callback_ = nullptr;
  callback_user_data_ = nullptr;
  signaled_ = false;
  return MLN_STATUS_OK;
}

auto NotificationSourceObject::begin_ready_drain(
  std::vector<mln_ready_endpoint>& out_endpoints
) -> mln_status {
  const std::scoped_lock lock(mutex_);
  if (closing_) {
    set_thread_error("notification source is closing");
    return MLN_STATUS_INVALID_STATE;
  }
  if (ready_drain_active_) {
    set_thread_error("notification source already has an active ready drain");
    return MLN_STATUS_INVALID_STATE;
  }
  ready_drain_active_ = true;
  try {
    out_endpoints.reserve(endpoints_.size());
    for (const auto& [id, endpoint] : endpoints_) {
      if (endpoint.ready) {
        out_endpoints.push_back(
          mln_ready_endpoint{
            .size = sizeof(mln_ready_endpoint), .kind = endpoint.kind, .id = id
          }
        );
      }
    }
  } catch (...) {
    ready_drain_active_ = false;
    throw;
  }
  return MLN_STATUS_OK;
}

auto NotificationSourceObject::commit_ready_drain(
  const std::vector<mln_ready_endpoint>& endpoints
) noexcept -> void {
  const std::scoped_lock lock(mutex_);
  for (const auto& endpoint : endpoints) {
    const auto found = endpoints_.find(endpoint.id);
    if (
      found != endpoints_.end() && found->second.kind == endpoint.kind &&
      !found->second.sticky
    ) {
      found->second.ready = false;
    }
  }
  ready_drain_active_ = false;
  recompute_signaled_locked();
}

auto NotificationSourceObject::abort_ready_drain() noexcept -> void {
  const std::scoped_lock lock(mutex_);
  ready_drain_active_ = false;
}

auto notification_source_from_handle(mln_notification_source source)
  -> std::shared_ptr<NotificationSourceObject> {
  return handle_table<NotificationSourceObject>().lease(source);
}

auto create_notification_source(mln_notification_source* out_source)
  -> mln_status {
  if (out_source == nullptr || *out_source != MLN_HANDLE_NULL) {
    set_thread_error("out_source must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_source = handle_table<NotificationSourceObject>().insert(
    std::make_shared<NotificationSourceObject>()
  );
  return MLN_STATUS_OK;
}

auto set_notification_callback(
  mln_notification_source source, mln_notification_callback callback,
  void* user_data
) -> mln_status {
  const auto live = notification_source_from_handle(source);
  return live == nullptr ? MLN_STATUS_INVALID_ARGUMENT
                         : live->set_callback(callback, user_data);
}

auto clear_notification_callback(mln_notification_source source) -> mln_status {
  const auto live = notification_source_from_handle(source);
  return live == nullptr ? MLN_STATUS_INVALID_ARGUMENT : live->clear_callback();
}

auto drain_notification_ready(
  mln_notification_source source, mln_ready_batch* out_batch
) -> mln_status {
  const auto live = notification_source_from_handle(source);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_batch == nullptr || *out_batch != MLN_HANDLE_NULL) {
    set_thread_error("out_batch must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto owned = std::make_shared<ReadyBatchObject>();
  const auto status = live->begin_ready_drain(owned->endpoints);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  try {
    *out_batch = handle_table<ReadyBatchObject>().insert(owned);
  } catch (...) {
    live->abort_ready_drain();
    throw;
  }
  live->commit_ready_drain(owned->endpoints);
  return MLN_STATUS_OK;
}

auto get_ready_batch(mln_ready_batch batch, mln_ready_batch_view* out_view)
  -> mln_status {
  const auto live = handle_table<ReadyBatchObject>().lease(batch);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_view == nullptr || out_view->size < sizeof(mln_ready_batch_view)) {
    set_thread_error("out_view must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_view = mln_ready_batch_view{
    .size = sizeof(mln_ready_batch_view),
    .endpoint_size = sizeof(mln_ready_endpoint),
    .endpoints = live->endpoints.empty() ? nullptr : live->endpoints.data(),
    .endpoint_count = live->endpoints.size()
  };
  return MLN_STATUS_OK;
}

auto release_ready_batch(mln_ready_batch batch) noexcept -> void {
  static_cast<void>(handle_table<ReadyBatchObject>().remove(batch));
}

auto close_notification_source(mln_notification_source source) -> mln_status {
  const auto live = notification_source_from_handle(source);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto status = live->begin_close();
  if (status != MLN_STATUS_OK) {
    return status;
  }
  static_cast<void>(handle_table<NotificationSourceObject>().remove(source));
  return MLN_STATUS_OK;
}

}  // namespace mln::core
