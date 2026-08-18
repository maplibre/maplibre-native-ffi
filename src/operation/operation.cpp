#include <utility>

#include "operation/operation.hpp"

#include "diagnostics/diagnostics.hpp"

namespace mln::core {

OperationObject::OperationObject(ResultCallback result)
    : result_callback_(std::move(result)) {}

auto OperationObject::set_terminal_callback(TerminalCallback callback) noexcept
  -> void {
  auto invoke_now = false;
  {
    const auto lock = std::scoped_lock{mutex_};
    if (completed_) {
      invoke_now = true;
    } else {
      terminal_callback_ = std::move(callback);
    }
  }
  if (invoke_now && callback) {
    try {
      callback();
    } catch (...) {
      // Terminal observation cannot reopen completed native work.
    }
  }
}

auto OperationObject::complete(
  mln_status status, std::string diagnostic, std::any result
) noexcept -> void {
  auto terminal = TerminalCallback{};
  auto deliver = ResultCallback{};
  {
    const auto lock = std::scoped_lock{mutex_};
    if (completed_) return;
    completed_ = true;
    terminal = std::move(terminal_callback_);
    deliver = std::move(result_callback_);
  }
  if (deliver) {
    try {
      deliver(status, std::move(diagnostic), std::move(result));
    } catch (...) {
      // Host completion delivery cannot reopen completed native work.
    }
  }
  if (terminal) {
    try {
      terminal();
    } catch (...) {
      // Terminal observation cannot reopen completed native work.
    }
  }
}

auto create_completion_operation(
  const mln_completion* descriptor, OperationObject::ResultCallback result,
  CompletionOperation& out
) -> mln_status {
  const auto status = validate_completion(descriptor);
  if (status != MLN_STATUS_OK) return status;
  try {
    out.completion = std::make_shared<Completion>(*descriptor);
    if (!result) {
      result = [completion = out.completion](
                 mln_status completion_status, std::string diagnostic, std::any
               ) {
        complete(completion, completion_status, std::move(diagnostic));
      };
    }
    out.operation = std::make_shared<OperationObject>(std::move(result));
  } catch (...) {
    out = {};
    set_thread_error("asynchronous completion state could not be allocated");
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

}  // namespace mln::core
