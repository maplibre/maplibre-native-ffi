#pragma once

#include <any>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>

#include "maplibre_native_c/operation.h"
#include "notification/notification.hpp"

namespace mln::core {
struct OperationResultTransfer {
  mln_status status;
  bool consume;
};

class OperationObject final {
 public:
  using CancelCallback = std::function<void()>;
  using TerminalCallback = std::function<void()>;

  OperationObject(std::uint32_t kind, bool cancellable, CancelCallback cancel);
  OperationObject(const OperationObject&) = delete;
  OperationObject(OperationObject&&) = delete;
  auto operator=(const OperationObject&) -> OperationObject& = delete;
  auto operator=(OperationObject&&) -> OperationObject& = delete;
  ~OperationObject() = default;

  auto publish(
    mln_operation self, std::shared_ptr<NotificationEndpoint> endpoint
  ) noexcept -> void;
  auto set_terminal_callback(TerminalCallback callback) noexcept -> void;
  auto complete(
    mln_status status, std::string diagnostic, std::any result
  ) noexcept -> void;
  auto cancel() -> mln_status;
  auto poll(bool* out_completed) -> mln_status;
  auto wait(std::int64_t timeout_ms, bool* out_completed) -> mln_status;
  auto get_status(mln_status* out_status) -> mln_status;
  auto copy_diagnostic(
    char* out_diagnostic, std::size_t diagnostic_capacity,
    std::size_t* out_diagnostic_size
  ) -> mln_status;
  auto discard_result() -> mln_status;
  auto release_observer() noexcept -> void;

  template <typename Result, typename Transfer>
  auto take_result(std::uint32_t expected_kind, Transfer&& transfer)
    -> mln_status {
    const std::scoped_lock lock(mutex_);
    if (kind_ != expected_kind) {
      set_operation_error(
        "operation result type does not match the take function"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (!completed_) {
      set_operation_error("operation is still pending");
      return MLN_STATUS_INVALID_STATE;
    }
    if (status_ != MLN_STATUS_OK) {
      set_operation_error(
        diagnostic_.empty() ? "operation did not complete successfully"
                            : diagnostic_.c_str()
      );
      return MLN_STATUS_INVALID_STATE;
    }
    if (!result_available_) {
      set_operation_error("operation result was already taken or discarded");
      return MLN_STATUS_INVALID_STATE;
    }
    auto* result = std::any_cast<Result>(&result_);
    if (result == nullptr) {
      set_operation_error("operation result storage has the wrong type");
      return MLN_STATUS_INVALID_STATE;
    }
    const auto transfer_result = std::forward<Transfer>(transfer)(*result);
    auto transfer_status = MLN_STATUS_OK;
    auto consume = true;
    if constexpr (
      std::is_same_v<
        std::remove_cvref_t<decltype(transfer_result)>, OperationResultTransfer>
    ) {
      transfer_status = transfer_result.status;
      consume = transfer_result.consume;
    } else {
      static_assert(
        std::is_same_v<
          std::remove_cvref_t<decltype(transfer_result)>, mln_status>
      );
      transfer_status = transfer_result;
    }
    if (transfer_status != MLN_STATUS_OK || !consume) {
      return transfer_status;
    }
    result_.reset();
    result_available_ = false;
    return MLN_STATUS_OK;
  }

 private:
  static auto set_operation_error(const char* message) noexcept -> void;
  auto finish_cancelled_locked(
    std::shared_ptr<NotificationEndpoint>& out_endpoint,
    CancelCallback& out_cancel, TerminalCallback& out_terminal
  ) noexcept -> void;

  std::mutex mutex_;
  std::condition_variable condition_;
  std::uint32_t kind_;
  mln_operation self_ = MLN_HANDLE_NULL;
  bool cancellable_;
  bool completed_ = false;
  bool observer_attached_ = true;
  bool result_available_ = false;
  mln_status status_ = MLN_STATUS_OK;
  std::string diagnostic_;
  std::any result_;
  CancelCallback cancel_callback_;
  std::shared_ptr<NotificationEndpoint> endpoint_;
  TerminalCallback terminal_callback_;
};

[[nodiscard]] auto lease_operation(mln_operation operation)
  -> std::shared_ptr<OperationObject>;

auto register_operation(
  const std::shared_ptr<NotificationSourceObject>& source, std::uint32_t kind,
  bool cancellable, OperationObject::CancelCallback cancel,
  mln_operation* out_operation, std::shared_ptr<OperationObject>& out_state
) -> mln_status;
auto abandon_operation(mln_operation operation) noexcept -> void;

auto poll_operation(mln_operation operation, bool* out_completed) -> mln_status;
auto wait_operation(
  mln_operation operation, std::int64_t timeout_ms, bool* out_completed
) -> mln_status;
auto cancel_operation(mln_operation operation) -> mln_status;
auto get_operation_status(mln_operation operation, mln_status* out_status)
  -> mln_status;
auto copy_operation_diagnostic(
  mln_operation operation, char* out_diagnostic,
  std::size_t diagnostic_capacity, std::size_t* out_diagnostic_size
) -> mln_status;
auto discard_operation_result(mln_operation operation) -> mln_status;
auto release_operation(mln_operation operation) noexcept -> void;

template <typename Result, typename Transfer>
auto take_operation_result(
  mln_operation operation, std::uint32_t expected_kind, Transfer&& transfer
) -> mln_status {
  const auto live = lease_operation(operation);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return live->take_result<Result>(
    expected_kind, std::forward<Transfer>(transfer)
  );
}

}  // namespace mln::core
