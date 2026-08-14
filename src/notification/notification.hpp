#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "maplibre_native_c/notification.h"

namespace mln::core {

struct ReadyBatchObject {
  std::vector<mln_ready_endpoint> endpoints;
};

class NotificationSourceObject;

class NotificationEndpoint final {
 public:
  NotificationEndpoint(
    std::shared_ptr<NotificationSourceObject> source, std::uint64_t id,
    std::uint32_t kind, bool sticky
  );
  NotificationEndpoint(const NotificationEndpoint&) = delete;
  NotificationEndpoint(NotificationEndpoint&&) = delete;
  auto operator=(const NotificationEndpoint&) -> NotificationEndpoint& = delete;
  auto operator=(NotificationEndpoint&&) -> NotificationEndpoint& = delete;
  ~NotificationEndpoint();

  auto mark_ready() const noexcept -> void;
  auto clear_ready() const noexcept -> void;
  auto detach() const noexcept -> void;
  [[nodiscard]] auto source() const noexcept
    -> std::shared_ptr<NotificationSourceObject>;

 private:
  std::shared_ptr<NotificationSourceObject> source_;
  std::uint32_t kind_;
  std::uint64_t id_;
  friend class NotificationSourceObject;
  mutable std::atomic_bool active_{false};
};

class NotificationSourceObject final
    : public std::enable_shared_from_this<NotificationSourceObject> {
 public:
  NotificationSourceObject() = default;
  NotificationSourceObject(const NotificationSourceObject&) = delete;
  NotificationSourceObject(NotificationSourceObject&&) = delete;
  auto operator=(const NotificationSourceObject&)
    -> NotificationSourceObject& = delete;
  auto operator=(NotificationSourceObject&&)
    -> NotificationSourceObject& = delete;
  ~NotificationSourceObject() = default;

  auto associate(std::uint64_t id, std::uint32_t kind, bool sticky)
    -> std::shared_ptr<NotificationEndpoint>;
  auto detach(
    std::uint64_t id, std::uint32_t kind, const NotificationEndpoint* owner
  ) noexcept -> void;
  auto mark_ready(
    std::uint64_t id, std::uint32_t kind, const NotificationEndpoint* owner
  ) noexcept -> void;
  auto clear_ready(
    std::uint64_t id, std::uint32_t kind, const NotificationEndpoint* owner
  ) noexcept -> void;

  auto set_callback(mln_notification_callback callback, void* user_data)
    -> mln_status;
  auto clear_callback() -> mln_status;
  auto begin_close() -> mln_status;

  auto begin_ready_drain(std::vector<mln_ready_endpoint>& out_endpoints)
    -> mln_status;
  auto commit_ready_drain(
    const std::vector<mln_ready_endpoint>& endpoints
  ) noexcept -> void;
  auto abort_ready_drain() noexcept -> void;

 private:
  struct EndpointState {
    std::uint32_t kind = 0;
    bool sticky = false;
    const NotificationEndpoint* owner = nullptr;
    bool ready = false;
  };

  struct CallbackInvocation {
    mln_notification_callback callback = nullptr;
    void* user_data = nullptr;
  };

  [[nodiscard]] auto prepare_callback_locked() noexcept -> CallbackInvocation;
  auto invoke(CallbackInvocation invocation) noexcept -> void;
  auto finish_callback() noexcept -> void;
  auto recompute_signaled_locked() noexcept -> void;

  std::mutex mutex_;
  std::condition_variable callback_condition_;
  struct EndpointKey {
    std::uint64_t id;
    std::uint32_t kind;
    [[nodiscard]] auto operator==(const EndpointKey&) const noexcept
      -> bool = default;
  };
  struct EndpointKeyHash {
    [[nodiscard]] auto operator()(const EndpointKey& key) const noexcept
      -> std::size_t {
      return std::hash<std::uint64_t>{}(
        key.id ^ (static_cast<std::uint64_t>(key.kind) << 32u)
      );
    }
  };
  std::unordered_map<EndpointKey, EndpointState, EndpointKeyHash> endpoints_;
  mln_notification_callback callback_ = nullptr;
  void* callback_user_data_ = nullptr;
  std::size_t callbacks_in_flight_ = 0;
  bool callback_enabled_ = true;
  bool signaled_ = false;
  bool ready_drain_active_ = false;
  bool closing_ = false;
};

[[nodiscard]] auto notification_source_from_handle(
  mln_notification_source source
) -> std::shared_ptr<NotificationSourceObject>;

auto create_notification_source(mln_notification_source* out_source)
  -> mln_status;
auto set_notification_callback(
  mln_notification_source source, mln_notification_callback callback,
  void* user_data
) -> mln_status;
auto clear_notification_callback(mln_notification_source source) -> mln_status;
auto drain_notification_ready(
  mln_notification_source source, mln_ready_batch* out_batch
) -> mln_status;
auto get_ready_batch(mln_ready_batch batch, mln_ready_batch_view* out_view)
  -> mln_status;
auto release_ready_batch(mln_ready_batch batch) noexcept -> void;
auto close_notification_source(mln_notification_source source) -> mln_status;

}  // namespace mln::core
