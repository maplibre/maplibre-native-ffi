#pragma once

#include <any>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "completion/completion.hpp"
#include "completion/completion_result.hpp"

namespace mln::core {

/** Internal terminal state shared by queued native work. */
class OperationObject final {
 public:
  using ResultCallback = std::function<void(mln_status, std::string, std::any)>;

  explicit OperationObject(ResultCallback result = {});
  OperationObject(const OperationObject&) = delete;
  OperationObject(OperationObject&&) = delete;
  auto operator=(const OperationObject&) -> OperationObject& = delete;
  auto operator=(OperationObject&&) -> OperationObject& = delete;
  ~OperationObject();

  auto complete(
    mln_status status, std::string diagnostic, std::any result
  ) noexcept -> void;

 private:
  std::mutex mutex_;
  bool completed_ = false;
  ResultCallback result_callback_;
};

struct CompletionOperation {
  using Delivery = std::function<void(
    const std::shared_ptr<Completion>&, mln_status, std::string, std::any
  )>;

  std::shared_ptr<OperationObject> operation;
  std::shared_ptr<Completion> completion;
};

// Pairs an operation with a completion that deliver resolves from the
// operation's result. deliver must not be empty.
auto create_completion_operation(
  const mln_completion* descriptor, CompletionOperation::Delivery deliver,
  CompletionOperation& out, bool command = false
) -> mln_status;

auto create_completion_operation(
  const mln_completion* descriptor, ValuelessCompletion valueless,
  CompletionOperation& out
) -> mln_status;

auto create_completion_operation(
  std::shared_ptr<Completion> completion, CompletionOperation::Delivery deliver,
  CompletionOperation& out
) -> mln_status;
auto create_completion_operation(
  std::shared_ptr<Completion> completion, ValuelessCompletion valueless,
  CompletionOperation& out
) -> mln_status;

}  // namespace mln::core
