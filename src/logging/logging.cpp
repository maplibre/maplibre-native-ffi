#include <cstdint>
#include <memory>
#include <string>

#include <mbgl/util/event.hpp>
#include <mbgl/util/logging.hpp>

#include "logging/logging.hpp"

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"

namespace {

class CallbackLogObserver final : public mln::Log::Observer {
 public:
  CallbackLogObserver(mln_log_callback callback, void* user_data)
      : callback_(callback), user_data_(user_data) {}

  auto onRecord(
    mln::EventSeverity severity, mln::Event event, std::int64_t code,
    const std::string& message
  ) -> bool override {
    if (callback_ == nullptr) {
      return false;
    }

    return callback_(
             user_data_, static_cast<std::uint32_t>(severity),
             static_cast<std::uint32_t>(event), code, message.c_str()
           ) != 0U;
  }

 private:
  mln_log_callback callback_ = nullptr;
  void* user_data_ = nullptr;
};

auto set_severity_async(
  std::uint32_t mask, mln_log_severity_mask bit, mln::EventSeverity severity
) -> void {
  mln::Log::useLogThread((mask & bit) != 0U, severity);
}

}  // namespace

namespace mln::core {

auto set_log_callback(mln_log_callback callback, void* user_data)
  -> mln_status {
  if (callback == nullptr) {
    mln::Log::removeObserver();
    return MLN_STATUS_OK;
  }

  mln::Log::setObserver(
    std::make_unique<CallbackLogObserver>(callback, user_data)
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
