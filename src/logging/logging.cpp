#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <mln/util/event.hpp>
#include <mln/util/logging.hpp>

#include "logging/logging.hpp"

#include "delivery/delivery_queue.hpp"
#include "diagnostics/diagnostics.hpp"
#include "execution/process_exit.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"

namespace {

class CallbackLogObserver final : public mln::Log::Observer {
 public:
  CallbackLogObserver(
    mln_log_callback callback, void* user_data,
    mln_user_data_release release_user_data
  )
      : callback_(callback),
        user_data_(user_data),
        release_user_data_(release_user_data) {}

  ~CallbackLogObserver() override {
    if (release_user_data_ == nullptr || mln::core::process_exiting()) {
      return;
    }
    try {
      release_user_data_(user_data_);
    } catch (...) {
      // Host release callbacks must not unwind through the C boundary.
    }
  }

  auto onRecord(
    mln::EventSeverity severity, mln::Event event, std::int64_t code,
    const std::string& message
  ) -> bool override {
    if (mln::core::process_exiting()) {
      // Consumed, so the platform logger stays quiet too.
      return true;
    }

    return callback_(
             user_data_, static_cast<std::uint32_t>(severity),
             static_cast<std::uint32_t>(event), code, message.c_str()
           ) != 0U;
  }

 private:
  mln_log_callback callback_ = nullptr;
  void* user_data_ = nullptr;
  mln_user_data_release release_user_data_ = nullptr;
};

// Copies each record into a delivery queue. A record the queue refuses goes to
// the platform logger.
class QueueLogObserver final : public mln::Log::Observer {
 public:
  QueueLogObserver(
    std::shared_ptr<mln::delivery::DeliveryQueueState> queue, void* user_data,
    mln::delivery::ItemPtr retired
  ) noexcept
      : queue_(std::move(queue)),
        user_data_(user_data),
        retired_(std::move(retired)) {}

  QueueLogObserver(const QueueLogObserver&) = delete;
  QueueLogObserver(QueueLogObserver&&) = delete;
  auto operator=(const QueueLogObserver&) -> QueueLogObserver& = delete;
  auto operator=(QueueLogObserver&&) -> QueueLogObserver& = delete;

  // Retirement follows the handler's final record, where callback mode runs
  // release_user_data.
  ~QueueLogObserver() override {
    if (mln::core::process_exiting()) {
      mln::delivery::forget_item(std::move(retired_));
      return;
    }
    static_cast<void>(queue_->push(retired_));
  }

  auto onRecord(
    mln::EventSeverity severity, mln::Event event, std::int64_t code,
    const std::string& message
  ) -> bool override {
    if (mln::core::process_exiting()) {
      // Consumed, so the platform logger stays quiet too.
      return true;
    }
    const auto record = mln_log_delivery{
      .severity = static_cast<std::uint32_t>(severity),
      .event = static_cast<std::uint32_t>(event),
      .code = code,
      .message = message.c_str(),
    };
    try {
      auto item = mln::delivery::make_item(
        MLN_DELIVERY_TYPE_LOG_RECORD, user_data_,
        [&](auto& arena, mln_delivery_payload& payload) {
          payload.log_record =
            mln::delivery::capture_mln_log_delivery(arena, record);
        }
      );
      return queue_->push(item);
    } catch (...) {
      return false;
    }
  }

 private:
  std::shared_ptr<mln::delivery::DeliveryQueueState> queue_;
  void* user_data_ = nullptr;
  mln::delivery::ItemPtr retired_;
};

auto set_severity_async(
  std::uint32_t mask, mln_log_severity_mask bit, mln::EventSeverity severity
) -> void {
  mln::Log::useLogThread((mask & bit) != 0U, severity);
}

}  // namespace

namespace mln::core {

auto set_log_callback(const mln_log_handler* handler) -> mln_status {
  if (handler == nullptr) {
    set_thread_error("log handler must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (handler->size < sizeof(mln_log_handler)) {
    set_thread_error("mln_log_handler.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto queued = handler->queue != MLN_HANDLE_NULL;
  if ((handler->callback == nullptr) == !queued) {
    set_thread_error(
      "log handler requires exactly one of a callback and a delivery queue"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (queued) {
    if (handler->release_user_data != nullptr) {
      set_thread_error("a queue log handler must not carry release_user_data");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto queue = mln::delivery::resolve_delivery_queue(handler->queue);
    if (queue == nullptr) return recorded_handle_fault_status();
    auto observer = std::make_unique<QueueLogObserver>(
      std::move(queue), handler->user_data,
      mln::delivery::make_light_item(
        MLN_DELIVERY_TYPE_RETIRED, handler->user_data
      )
    );
    watch_process_exit();
    mln::Log::setObserver(std::move(observer));
    return MLN_STATUS_OK;
  }

  watch_process_exit();

  mln::Log::setObserver(
    std::make_unique<CallbackLogObserver>(
      handler->callback, handler->user_data, handler->release_user_data
    )
  );
  return MLN_STATUS_OK;
}

auto clear_log_callback() -> mln_status {
  mln::Log::removeObserver();
  return MLN_STATUS_OK;
}

auto set_log_async_severity_mask(std::uint32_t mask) -> mln_status {
  if ((mask & ~MLN_LOG_SEVERITY_MASK_ALL) != 0U) {
    set_thread_error("log async severity mask contains unknown bits");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  set_severity_async(
    mask, MLN_LOG_SEVERITY_MASK_INFO, mln::EventSeverity::Info
  );
  set_severity_async(
    mask, MLN_LOG_SEVERITY_MASK_WARNING, mln::EventSeverity::Warning
  );
  set_severity_async(
    mask, MLN_LOG_SEVERITY_MASK_ERROR, mln::EventSeverity::Error
  );
  return MLN_STATUS_OK;
}

}  // namespace mln::core
