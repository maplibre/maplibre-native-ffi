#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <mln/util/event.hpp>
#include <mln/util/logging.hpp>

#include "logging/logging.hpp"

#include "delivery/queued_registration.hpp"
#include "diagnostics/diagnostics.hpp"
#include "execution/forbid_reentry.hpp"
#include "execution/process_exit.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"

namespace {

// MapLibre Native runs each observer call under its log mutex, which is not
// recursive, and nearly every C API call can log.
constexpr auto log_reentry_diagnostic =
  "the C API cannot be called from a log callback, wake, or release";

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
    const auto forbid = mln::core::ForbidReentry{log_reentry_diagnostic};
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

    const auto forbid = mln::core::ForbidReentry{log_reentry_diagnostic};
    return callback_(
      user_data_, static_cast<mln_log_severity>(severity),
      static_cast<mln_log_event>(event), code,
      mln_buffer_view{.data = message.data(), .size = message.size()}
    );
  }

 private:
  mln_log_callback callback_ = nullptr;
  void* user_data_ = nullptr;
  mln_user_data_release release_user_data_ = nullptr;
};

// Consumes each record and queues a copy, or leaves a record the queue
// refuses to the platform logger.
auto queue_log_record(
  void* registration, mln_log_severity severity, mln_log_event event,
  std::int64_t code, mln_buffer_view message
) noexcept -> bool {
  const auto record = mln::delivery::LogArguments{
    .severity = severity,
    .event = event,
    .code = code,
    .message = message,
  };
  return static_cast<const mln::delivery::QueuedRegistration*>(registration)
    ->push(record);
}

auto set_severity_async(
  std::uint32_t mask, mln_log_severity_flag bit, mln::EventSeverity severity
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
  auto queue = std::shared_ptr<mln::delivery::DeliveryQueueState>{};
  const auto queue_status = mln::delivery::resolve_registration_queue(
    "log handler", handler->queue, handler->callback != nullptr, queue
  );
  if (queue_status != MLN_STATUS_OK) return queue_status;

  watch_process_exit();
  if (queue != nullptr) {
    // A replacement destroys the observer under the log mutex, so the
    // retirement's wake follows the log callback restrictions, as does the
    // wake's release when the observer holds the last share of a released
    // queue.
    using Registration = mln::delivery::QueuedRegistration;
    auto registration = std::make_unique<Registration>(
      std::move(queue), handler->user_data, handler->release_user_data,
      std::initializer_list<Registration::Callback>{
        Registration::callback<mln::delivery::LogArguments>(handler->callback)
      }
    );
    auto observer = std::make_unique<CallbackLogObserver>(
      &queue_log_record, registration.get(),
      &mln::delivery::QueuedRegistration::retire
    );
    static_cast<void>(registration.release());
    mln::Log::setObserver(std::move(observer));
    return MLN_STATUS_OK;
  }
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
  if ((mask & ~MLN_LOG_SEVERITY_FLAG_ALL) != 0U) {
    set_thread_error("log async severity mask contains unknown bits");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  set_severity_async(
    mask, MLN_LOG_SEVERITY_FLAG_INFO, mln::EventSeverity::Info
  );
  set_severity_async(
    mask, MLN_LOG_SEVERITY_FLAG_WARNING, mln::EventSeverity::Warning
  );
  set_severity_async(
    mask, MLN_LOG_SEVERITY_FLAG_ERROR, mln::EventSeverity::Error
  );
  return MLN_STATUS_OK;
}

}  // namespace mln::core
