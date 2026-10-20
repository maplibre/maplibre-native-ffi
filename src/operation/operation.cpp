#include <utility>

#include "operation/operation.hpp"

#include "diagnostics/diagnostics.hpp"

namespace mln::core {

OperationObject::OperationObject(ResultCallback result)
    : result_callback_(std::move(result)) {}

OperationObject::~OperationObject() { complete(MLN_STATUS_CANCELLED, {}, {}); }

auto OperationObject::complete(
  mln_status status, std::string diagnostic, std::any result
) noexcept -> void {
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
}

auto create_completion_operation(
  const mln_completion* descriptor, CompletionOperation::Delivery deliver,
  CompletionOperation& out, bool command
) -> mln_status {
  const auto status = command
                        ? make_command_completion(descriptor, out.completion)
                        : make_completion(descriptor, out.completion);
  if (status != MLN_STATUS_OK) return status;
  return create_completion_operation(out.completion, std::move(deliver), out);
}

auto create_completion_operation(
  std::shared_ptr<Completion> completion, CompletionOperation::Delivery deliver,
  CompletionOperation& out
) -> mln_status {
  out.completion = std::move(completion);
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

auto create_completion_operation(
  std::shared_ptr<Completion> completion, ValuelessCompletion valueless,
  CompletionOperation& out
) -> mln_status {
  return create_completion_operation(
    std::move(completion),
    [valueless](
      const std::shared_ptr<Completion>& state, mln_status status,
      std::string diagnostic, std::any
    ) { valueless.complete(state, status, std::move(diagnostic)); },
    out
  );
}

}  // namespace mln::core
