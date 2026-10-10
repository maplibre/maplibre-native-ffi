#include <cassert>
#include <utility>

#include "completion/completion.hpp"

#include "completion/completion_result.hpp"
#include "diagnostics/diagnostics.hpp"
#include "execution/process_exit.hpp"

namespace mln::core {
namespace {

auto invoke_completion(
  const mln_completion& descriptor, mln_status status,
  std::uint32_t disposition, std::uint64_t generation,
  const std::string& diagnostic, const void* value, std::size_t value_count,
  std::uint32_t value_size
) noexcept -> void {
  const auto result = mln_completion_result{
    .size = sizeof(mln_completion_result),
    .status = status,
    .disposition = disposition,
    .value_size = value == nullptr ? 0 : value_size,
    .generation = generation,
    .diagnostic =
      mln_buffer_view{.data = diagnostic.data(), .size = diagnostic.size()},
    .value = value,
    .value_count = value_count,
  };
  if (process_exiting()) return;
  try {
    descriptor.callback(descriptor.user_data, &result);
  } catch (...) {
    // Host callbacks must not unwind through the C boundary.
  }
}

}  // namespace

auto ErasedCompletionValue::deliver(
  const mln_completion& descriptor, const void* value, std::size_t count,
  std::uint32_t size
) noexcept -> void {
  invoke_completion(
    descriptor, MLN_STATUS_OK, MLN_COMMAND_DISPOSITION_COMMITTED, 0, {}, value,
    count, size
  );
}

auto deliver_failure(
  const mln_completion& descriptor, mln_status status,
  const std::string& diagnostic
) noexcept -> void {
  if (status == MLN_STATUS_OK) {
    assert(false && "a failure delivery reported success");
    invoke_completion(
      descriptor, MLN_STATUS_NATIVE_ERROR, MLN_COMMAND_DISPOSITION_COMMITTED, 0,
      "native completion reported success without its result", nullptr, 0, 0
    );
    return;
  }
  invoke_completion(
    descriptor, status, MLN_COMMAND_DISPOSITION_COMMITTED, 0, diagnostic,
    nullptr, 0, 0
  );
}

auto ValuelessCompletion::deliver(
  const mln_completion& descriptor, mln_status status,
  const std::string& diagnostic
) const noexcept -> void {
  if (status == MLN_STATUS_OK) {
    ErasedCompletionValue::deliver(descriptor, nullptr, 0, 0);
  } else {
    deliver_failure(descriptor, status, diagnostic);
  }
}

auto ValuelessCompletion::complete(
  const std::shared_ptr<Completion>& completion, mln_status status,
  std::string diagnostic
) const noexcept -> void {
  completion->resolve([self = *this, status,
                       diagnostic = std::move(diagnostic)](
                        const mln_completion& descriptor
                      ) { self.deliver(descriptor, status, diagnostic); });
}

Completion::Completion(const mln_completion& descriptor)
    : descriptor_(descriptor) {}

Completion::~Completion() {
  auto abandoned = false;
  {
    const std::scoped_lock lock(mutex_);
    abandoned = state_ == State::Accepted;
  }
  if (abandoned) {
    resolve([diagnostic = std::string{"asynchronous work was abandoned"}](
              const mln_completion& descriptor
            ) {
      invoke_completion(
        descriptor, MLN_STATUS_CANCELLED, MLN_COMMAND_DISPOSITION_CANCELLED, 0,
        diagnostic, nullptr, 0, 0
      );
    });
  }
}

auto Completion::accept() noexcept -> void {
  auto pending = Delivery{};
  {
    const std::scoped_lock lock(mutex_);
    if (state_ != State::Pending) {
      return;
    }
    if (pending_) {
      state_ = State::Resolved;
      pending = std::move(pending_);
    } else {
      state_ = State::Accepted;
    }
  }
  if (pending) {
    deliver(std::move(pending));
  }
}

auto Completion::reject() noexcept -> void {
  const std::scoped_lock lock(mutex_);
  if (state_ == State::Pending) {
    state_ = State::Rejected;
    pending_ = {};
    descriptor_ = {};
  }
}

auto Completion::resolve(Delivery delivery) noexcept -> void {
  auto invoke_now = false;
  {
    const std::scoped_lock lock(mutex_);
    switch (state_) {
      case State::Pending:
        if (!pending_) {
          pending_ = std::move(delivery);
        }
        return;
      case State::Accepted:
        state_ = State::Resolved;
        invoke_now = true;
        break;
      case State::Rejected:
      case State::Resolved:
        return;
    }
  }
  if (invoke_now) {
    deliver(std::move(delivery));
  }
}

auto Completion::deliver(Delivery delivery) noexcept -> void {
  try {
    delivery(descriptor_);
  } catch (...) {
    // Completion delivery cannot reopen terminal native work.
  }
  release();
}

auto Completion::release() noexcept -> void {
  const auto release_user_data = descriptor_.release_user_data;
  const auto user_data = descriptor_.user_data;
  descriptor_ = {};
  if (release_user_data != nullptr && !process_exiting()) {
    try {
      release_user_data(user_data);
    } catch (...) {
      // Release callbacks must not unwind through the C boundary.
    }
  }
}

auto validate_completion(const mln_completion* completion) -> mln_status {
  if (completion == nullptr) {
    set_thread_error("completion must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (completion->size < sizeof(mln_completion)) {
    set_thread_error("mln_completion.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (completion->callback == nullptr) {
    set_thread_error("completion callback must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto complete_failure(
  const std::shared_ptr<Completion>& completion, mln_status status,
  std::string diagnostic
) noexcept -> void {
  completion->resolve([status, diagnostic = std::move(diagnostic)](
                        const mln_completion& descriptor
                      ) { deliver_failure(descriptor, status, diagnostic); });
}

auto complete_command(
  const std::shared_ptr<Completion>& completion, std::uint32_t disposition,
  mln_status status, std::uint64_t generation, std::string diagnostic
) noexcept -> void {
  completion->resolve(
    [disposition, status, generation,
     diagnostic = std::move(diagnostic)](const mln_completion& descriptor) {
      invoke_completion(
        descriptor, status, disposition, generation, diagnostic, nullptr, 0, 0
      );
    }
  );
}

}  // namespace mln::core
