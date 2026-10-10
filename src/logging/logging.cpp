#include <cstdint>
#include <memory>
#include <string>

#include <mln/util/event.hpp>
#include <mln/util/logging.hpp>

#include "logging/logging.hpp"

#include "diagnostics/diagnostics.hpp"
#include "execution/process_exit.hpp"
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
  if (handler->callback == nullptr) {
    set_thread_error("log handler callback must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
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
