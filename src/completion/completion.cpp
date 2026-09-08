#include <utility>

#include "completion/completion.hpp"

#include "diagnostics/diagnostics.hpp"

namespace mln::core {

auto invoke_completion(
  const mln_completion& descriptor, mln_status status,
  std::uint32_t disposition, std::uint64_t generation,
  const std::string& diagnostic, const void* value, std::size_t value_count
) noexcept -> void {
  const auto result = mln_completion_result{
    .size = sizeof(mln_completion_result),
    .status = status,
    .disposition = disposition,
    .reserved = 0,
    .generation = generation,
    .diagnostic =
      mln_buffer_view{.data = diagnostic.data(), .size = diagnostic.size()},
    .value = value,
    .value_count = value_count,
  };
  try {
    descriptor.callback(descriptor.user_data, &result);
  } catch (...) {
    // Host callbacks must not unwind through the C boundary.
  }
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
        diagnostic, nullptr, 0
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
  if (release_user_data != nullptr) {
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

auto complete(
  const std::shared_ptr<Completion>& completion, mln_status status,
  std::string diagnostic, const void* value, std::size_t value_count
) noexcept -> void {
  completion->resolve([status, diagnostic = std::move(diagnostic), value,
                       value_count](const mln_completion& descriptor) {
    invoke_completion(
      descriptor, status, MLN_COMMAND_DISPOSITION_COMMITTED, 0, diagnostic,
      value, value_count
    );
  });
}

auto complete_command(
  const std::shared_ptr<Completion>& completion, std::uint32_t disposition,
  mln_status status, std::uint64_t generation, std::string diagnostic
) noexcept -> void {
  completion->resolve(
    [disposition, status, generation,
     diagnostic = std::move(diagnostic)](const mln_completion& descriptor) {
      invoke_completion(
        descriptor, status, disposition, generation, diagnostic, nullptr, 0
      );
    }
  );
}

}  // namespace mln::core
