#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "completion/completion.hpp"

#include "completion/completion_result.hpp"
#include "diagnostics/diagnostics.hpp"
#include "execution/process_exit.hpp"
#include "handles/handle_table.hpp"

namespace mln::core {
namespace {

// The diagnostic of a queued completion whose result could not be copied. It
// has static storage, so the fallback delivery needs no allocation.
constexpr char uncopyable_result[] =
  "the completion result could not be copied into its delivery queue";

template <bool Writing>
auto copy_result(
  delivery::Arena<Writing>& arena, const mln_completion_result& source,
  const delivery::CompletionCapture* capture
) -> mln_completion_result {
  auto copy = source;
  copy.diagnostic = delivery::buffer(arena, source.diagnostic);
  copy.value = nullptr;
  if (source.value == nullptr && source.value_count == 0) return copy;
  if (capture == nullptr) {
    throw std::invalid_argument{"completion value has no capture"};
  }
  if constexpr (Writing) {
    copy.value = capture->write(arena, source.value, source.value_count);
  } else {
    capture->measure(arena, source.value, source.value_count);
  }
  return copy;
}

}  // namespace

auto CompletionSink::deliver(
  mln_status status, std::uint32_t disposition, std::uint64_t generation,
  std::string_view diagnostic, const void* value, std::size_t value_count,
  std::uint32_t value_size, const delivery::CompletionCapture* capture
) const noexcept -> void {
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
  if (delivered_) return;
  delivered_ = true;
  if (process_exiting()) return;
  if (queue_ != nullptr) {
    deliver_queued(result, capture);
    return;
  }
  if (descriptor_.callback == nullptr) return;
  try {
    descriptor_.callback(descriptor_.user_data, &result);
  } catch (...) {
    // Host callbacks must not unwind through the C boundary.
  }
}

auto CompletionSink::deliver_queued(
  const mln_completion_result& result,
  const delivery::CompletionCapture* capture
) const noexcept -> void {
  auto item = delivery::ItemPtr{};
  try {
    item = delivery::make_item(
      MLN_DELIVERY_TYPE_COMPLETION, user_data_,
      [&](auto& arena, mln_delivery_payload& payload) {
        payload.completion = copy_result(arena, result, capture);
      }
    );
    if (
      capture != nullptr && result.status == MLN_STATUS_OK &&
      result.value != nullptr
    ) {
      item->discard = capture->discard_item;
    }
  } catch (...) {
    // The fallback reports the failure, and nothing keeps the source's owned
    // values, so they are disposed here.
    if (
      capture != nullptr && result.status == MLN_STATUS_OK &&
      result.value != nullptr
    ) {
      capture->discard(result.value, result.value_count);
    }
    item = std::move(fallback_);
    if (item == nullptr) return;
    item->view.payload.completion = mln_completion_result{
      .size = sizeof(mln_completion_result),
      .status = MLN_STATUS_NATIVE_ERROR,
      .disposition = result.disposition,
      .value_size = 0,
      .generation = result.generation,
      .diagnostic =
        mln_buffer_view{
          .data = uncopyable_result, .size = sizeof(uncopyable_result) - 1
        },
      .value = nullptr,
      .value_count = 0,
    };
  }
  // A released queue refuses the item, which disposes its owned values.
  if (!queue_->push(item)) item.reset();
}

auto ErasedCompletionValue::deliver(
  const CompletionSink& sink, const void* value, std::size_t count,
  std::uint32_t size, const delivery::CompletionCapture* capture
) noexcept -> void {
  sink.deliver(
    MLN_STATUS_OK, MLN_COMMAND_DISPOSITION_COMMITTED, 0, {}, value, count, size,
    capture
  );
}

auto deliver_failure(
  const CompletionSink& sink, mln_status status, const std::string& diagnostic
) noexcept -> void {
  if (status == MLN_STATUS_OK) {
    assert(false && "a failure delivery reported success");
    sink.deliver(
      MLN_STATUS_NATIVE_ERROR, MLN_COMMAND_DISPOSITION_COMMITTED, 0,
      "native completion reported success without its result", nullptr, 0, 0,
      nullptr
    );
    return;
  }
  sink.deliver(
    status, MLN_COMMAND_DISPOSITION_COMMITTED, 0, diagnostic, nullptr, 0, 0,
    nullptr
  );
}

auto ValuelessCompletion::deliver(
  const CompletionSink& sink, mln_status status, const std::string& diagnostic
) const noexcept -> void {
  if (status == MLN_STATUS_OK) {
    ErasedCompletionValue::deliver(sink, nullptr, 0, 0, nullptr);
  } else {
    deliver_failure(sink, status, diagnostic);
  }
}

auto ValuelessCompletion::complete(
  const std::shared_ptr<Completion>& completion, mln_status status,
  std::string diagnostic
) const noexcept -> void {
  completion->resolve([self = *this, status,
                       diagnostic =
                         std::move(diagnostic)](const CompletionSink& sink) {
    self.deliver(sink, status, diagnostic);
  });
}

Completion::Completion(CompletionSink sink) noexcept : sink_(std::move(sink)) {}

Completion::~Completion() {
  auto abandoned = false;
  {
    const std::scoped_lock lock(mutex_);
    abandoned = state_ == State::Accepted;
  }
  if (abandoned) {
    resolve([diagnostic = std::string{"asynchronous work was abandoned"}](
              const CompletionSink& sink
            ) {
      sink.deliver(
        MLN_STATUS_CANCELLED, MLN_COMMAND_DISPOSITION_CANCELLED, 0, diagnostic,
        nullptr, 0, 0, nullptr
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
  auto discarded = CompletionSink{};
  {
    const std::scoped_lock lock(mutex_);
    if (state_ == State::Pending) {
      state_ = State::Rejected;
      pending_ = {};
      discarded = std::move(sink_);
    }
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
    delivery(sink_);
  } catch (...) {
    // Completion delivery cannot reopen terminal native work, but a delivery
    // that threw before reaching the sink still owes the host its result.
    sink_.deliver(
      MLN_STATUS_NATIVE_ERROR, MLN_COMMAND_DISPOSITION_FAILED, 0,
      "the completion result could not be delivered", nullptr, 0, 0, nullptr
    );
  }
  release();
}

auto Completion::release() noexcept -> void {
  auto sink = std::move(sink_);
  sink_ = CompletionSink{};
  // A queue completion owns nothing of the host's to release.
  if (sink.queue_ != nullptr) return;
  const auto release_user_data = sink.descriptor_.release_user_data;
  const auto user_data = sink.descriptor_.user_data;
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
  const auto queued = completion->queue != MLN_HANDLE_NULL;
  if ((completion->callback == nullptr) == !queued) {
    set_thread_error(
      "completion requires exactly one of a callback and a delivery queue"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (queued) {
    if (completion->release_user_data != nullptr) {
      set_thread_error("a queue completion must not carry release_user_data");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (delivery::resolve_delivery_queue(completion->queue) == nullptr) {
      return recorded_handle_fault_status();
    }
  }
  return MLN_STATUS_OK;
}

auto accept_completion(
  const mln_completion* descriptor, std::shared_ptr<Completion>& out
) -> mln_status {
  const auto status = validate_completion(descriptor);
  if (status != MLN_STATUS_OK) return status;
  auto sink = CompletionSink{};
  try {
    if (descriptor->queue != MLN_HANDLE_NULL) {
      sink.queue_ = delivery::resolve_delivery_queue(descriptor->queue);
      // The queue was live a moment ago; a release in between reports as a
      // released queue.
      if (sink.queue_ == nullptr) return recorded_handle_fault_status();
      sink.user_data_ = descriptor->user_data;
      sink.fallback_ = delivery::make_light_item(
        MLN_DELIVERY_TYPE_COMPLETION, descriptor->user_data
      );
    } else {
      sink.descriptor_ = *descriptor;
    }
    out = std::make_shared<Completion>(std::move(sink));
  } catch (...) {
    out = nullptr;
    set_thread_error("asynchronous completion state could not be allocated");
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

auto complete_failure(
  const std::shared_ptr<Completion>& completion, mln_status status,
  std::string diagnostic
) noexcept -> void {
  completion->resolve([status, diagnostic = std::move(diagnostic)](
                        const CompletionSink& sink
                      ) { deliver_failure(sink, status, diagnostic); });
}

auto complete_command(
  const std::shared_ptr<Completion>& completion, std::uint32_t disposition,
  mln_status status, std::uint64_t generation, std::string diagnostic
) noexcept -> void {
  completion->resolve([disposition, status, generation,
                       diagnostic =
                         std::move(diagnostic)](const CompletionSink& sink) {
    sink.deliver(
      status, disposition, generation, diagnostic, nullptr, 0, 0, nullptr
    );
  });
}

}  // namespace mln::core
