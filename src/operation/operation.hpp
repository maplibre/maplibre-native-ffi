#pragma once

#include <any>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "completion/completion.hpp"

namespace mln::core {

/** Internal terminal state shared by queued native work. */
class OperationObject final {
 public:
  using TerminalCallback = std::function<void()>;
  using ResultCallback = std::function<void(mln_status, std::string, std::any)>;

  explicit OperationObject(ResultCallback result = {});
  OperationObject(const OperationObject&) = delete;
  OperationObject(OperationObject&&) = delete;
  auto operator=(const OperationObject&) -> OperationObject& = delete;
  auto operator=(OperationObject&&) -> OperationObject& = delete;
  ~OperationObject() = default;

  auto set_terminal_callback(TerminalCallback callback) noexcept -> void;
  auto complete(
    mln_status status, std::string diagnostic, std::any result
  ) noexcept -> void;

 private:
  std::mutex mutex_;
  bool completed_ = false;
  TerminalCallback terminal_callback_;
  ResultCallback result_callback_;
};

struct CompletionOperation {
  std::shared_ptr<OperationObject> operation;
  std::shared_ptr<Completion> completion;
};

auto create_completion_operation(
  const mln_completion* descriptor, OperationObject::ResultCallback result,
  CompletionOperation& out
) -> mln_status;

}  // namespace mln::core
