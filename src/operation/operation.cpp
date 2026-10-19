#include <utility>

#include "operation/operation.hpp"

#include "diagnostics/diagnostics.hpp"

namespace mln::core {
namespace {

auto observe_terminal(OperationObject::TerminalCallback callback) noexcept
  -> void {
  if (!callback) return;
  try {
    callback();
  } catch (...) {
    // Terminal observation cannot reopen completed native work.
  }
}

}  // namespace

OperationObject::OperationObject(ResultCallback result)
    : result_callback_(std::move(result)) {}

OperationObject::~OperationObject() { complete(MLN_STATUS_CANCELLED, {}, {}); }

auto OperationObject::set_terminal_callback(
  TerminalCallback callback, bool work_pending
) noexcept -> void {
  auto invoke_now = false;
  {
    const auto lock = std::scoped_lock{mutex_};
    work_pending_ = work_pending;
    if (delivery_finished_ && !work_pending_) {
      invoke_now = true;
    } else {
      terminal_callback_ = std::move(callback);
    }
  }
  if (invoke_now) observe_terminal(std::move(callback));
}

auto OperationObject::retire_work() noexcept -> void {
  auto terminal = TerminalCallback{};
  {
    const auto lock = std::scoped_lock{mutex_};
    work_pending_ = false;
    if (delivery_finished_) terminal = std::move(terminal_callback_);
  }
  observe_terminal(std::move(terminal));
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
    deliver = std::move(result_callback_);
  }
  if (deliver) {
    try {
      deliver(status, std::move(diagnostic), std::move(result));
    } catch (...) {
      // Host completion delivery cannot reopen completed native work.
    }
  }
  deliver = {};
  {
    const auto lock = std::scoped_lock{mutex_};
    delivery_finished_ = true;
    if (!work_pending_) terminal = std::move(terminal_callback_);
  }
  observe_terminal(std::move(terminal));
}

auto create_completion_operation(
  const mln_completion* descriptor, CompletionOperation::Delivery deliver,
  CompletionOperation& out, bool command
) -> mln_status {
  const auto status = command
                        ? make_command_completion(descriptor, out.completion)
                        : make_completion(descriptor, out.completion);
  if (status != MLN_STATUS_OK) return status;
  try {
    out.operation = std::make_shared<OperationObject>(
      [completion = out.completion, deliver = std::move(deliver)](
        mln_status completion_status, std::string diagnostic, std::any result
      ) {
        deliver(
          completion, completion_status, std::move(diagnostic),
          std::move(result)
        );
      }
    );
  } catch (...) {
    out = {};
    set_thread_error("asynchronous completion state could not be allocated");
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

auto create_completion_operation(
  const mln_completion* descriptor, ValuelessCompletion valueless,
  CompletionOperation& out
) -> mln_status {
  return create_completion_operation(
    descriptor,
    [valueless](
      const std::shared_ptr<Completion>& completion, mln_status status,
      std::string diagnostic, std::any
    ) { valueless.complete(completion, status, std::move(diagnostic)); },
    out
  );
}

}  // namespace mln::core
