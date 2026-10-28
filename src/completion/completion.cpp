#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <mln/util/logging.hpp>

#include "completion/completion.hpp"

#include "completion/completion_result.hpp"
#include "delivery/queued_registration.hpp"
#include "diagnostics/diagnostics.hpp"
#include "execution/process_exit.hpp"
#include "handles/handle_table.hpp"

namespace mln::core {
namespace {

// Small results and diagnostics share the storage reserved for fallback.
constexpr auto reserved_result_bytes = std::size_t{256};

// Marks a queue item as a call to the completion's callback and release.
auto set_completion_item(
  delivery::DeliveryItem& item, const mln_completion& descriptor
) noexcept -> void {
  item.user_data = descriptor.user_data;
  item.callback = reinterpret_cast<delivery::HostFunction>(descriptor.callback);
  item.release = descriptor.release_user_data;
  item.invoke = [](delivery::DeliveryItem& call) noexcept {
    reinterpret_cast<mln_completion_callback>(call.callback)(
      call.user_data, static_cast<const mln_completion_result*>(call.arguments)
    );
  };
}

// Places the result record first in an item's arena, where the fallback item
// keeps it too.
template <bool Writing>
auto place_result(delivery::Arena<Writing>& arena) -> mln_completion_result* {
  return arena.template allocate<mln_completion_result>(1);
}

// Static storage keeps fallback delivery independent of allocation.
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
  mln_status status, mln_command_disposition disposition,
  CommandGenerations generations, std::string_view diagnostic,
  const void* value, std::size_t value_count, std::uint32_t value_size,
  const delivery::CompletionCapture* capture, std::shared_ptr<const void> owner
) const noexcept -> void {
  const auto result = mln_completion_result{
    .size = sizeof(mln_completion_result),
    .status = status,
    .disposition = disposition,
    .value_size = value == nullptr ? 0 : value_size,
    .generation = generations.snapshot,
    .render_update_generation = generations.render_update,
    .diagnostic =
      mln_buffer_view{
        .data = diagnostic.empty() ? "" : diagnostic.data(),
        .size = diagnostic.size()
      },
    .value = value,
    .value_count = value_count,
  };
  assert(!owner_->delivered_);
  owner_->delivered_ = true;
  if (process_exiting()) return;
  if (queue_ != nullptr) {
    deliver_queued(result, capture, std::move(owner));
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
  const delivery::CompletionCapture* capture, std::shared_ptr<const void> owner
) const noexcept -> void {
  auto item = delivery::ItemPtr{};
  try {
    const auto borrow_bytes = owner != nullptr;
    const auto header = borrow_bytes ? sizeof(delivery::OwningDeliveryItem)
                                     : sizeof(delivery::DeliveryItem);
    auto measure =
      delivery::Arena<false>{.offset = header, .borrow_bytes = borrow_bytes};
    static_cast<void>(place_result(measure));
    static_cast<void>(copy_result(measure, result, capture));
    const auto reserved_size =
      sizeof(delivery::DeliveryItem) + reserved_result_bytes;
    // The fallback item cannot hold an owner, however small the copy.
    if (borrow_bytes) {
      item = delivery::ItemPtr{
        delivery::detail::allocate_owning_item(measure.offset, std::move(owner))
      };
    } else if (measure.offset <= reserved_size) {
      item = std::move(owner_->fallback_);
    } else {
      item = delivery::ItemPtr{delivery::detail::allocate_item(measure.offset)};
    }
    set_completion_item(*item, descriptor_);
    auto arena = delivery::Arena<true>{
      .storage = reinterpret_cast<std::byte*>(item.get()),
      .offset = header,
      .capacity = measure.offset,
      .borrow_bytes = borrow_bytes,
    };
    auto* record = place_result(arena);
    item->arguments = record;
    *record = copy_result(arena, result, capture);
  } catch (...) {
    // The fallback reports the failure.
    if (item == nullptr || item->arguments == nullptr) {
      item = std::move(owner_->fallback_);
    }
    if (item == nullptr) return;
    *static_cast<mln_completion_result*>(item->arguments) =
      mln_completion_result{
        .size = sizeof(mln_completion_result),
        .status = MLN_STATUS_NATIVE_ERROR,
        .disposition = result.disposition,
        .value_size = 0,
        .generation = result.generation,
        .render_update_generation = result.render_update_generation,
        .diagnostic =
          mln_buffer_view{
            .data = uncopyable_result, .size = sizeof(uncopyable_result) - 1
          },
        .value = nullptr,
        .value_count = 0,
    };
  }
  // A released queue refuses the item.
  if (!queue_->push(item, held_)) item.reset();
}

auto ErasedCompletionValue::deliver(
  const CompletionSink& sink, const void* value, std::size_t count,
  std::uint32_t size, const delivery::CompletionCapture* capture,
  std::shared_ptr<const void> owner
) noexcept -> void {
  sink.deliver(
    MLN_STATUS_OK, MLN_COMMAND_DISPOSITION_COMMITTED, {}, {}, value, count,
    size, capture, std::move(owner)
  );
}

auto deliver_failure(
  const CompletionSink& sink, mln_status status, const std::string& diagnostic
) noexcept -> void {
  if (status == MLN_STATUS_OK) {
    assert(false && "a failure delivery reported success");
    sink.deliver(
      MLN_STATUS_NATIVE_ERROR, MLN_COMMAND_DISPOSITION_COMMITTED, {},
      "native completion reported success without its result", nullptr, 0, 0,
      nullptr
    );
    return;
  }
  sink.deliver(
    status, MLN_COMMAND_DISPOSITION_COMMITTED, {}, diagnostic, nullptr, 0, 0,
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

Completion::Completion(CompletionSink sink, delivery::ItemPtr fallback) noexcept
    : sink_(std::move(sink)), fallback_(std::move(fallback)) {
  sink_.owner_ = this;
}

Completion::~Completion() {
  if (state_.load() == State::Accepted) {
    sink_.deliver(
      MLN_STATUS_CANCELLED, MLN_COMMAND_DISPOSITION_CANCELLED, {},
      "asynchronous work was abandoned", nullptr, 0, 0, nullptr
    );
    release();
  }
}

auto Completion::accept() noexcept -> void {
  auto expected = State::Created;
  state_.compare_exchange_strong(expected, State::Accepted);
}

auto Completion::revoke() noexcept -> void {
  auto expected = State::Accepted;
  if (state_.compare_exchange_strong(expected, State::Created)) {
    submission_lease_.reset();
  }
}

auto Completion::set_submission_lease(ControlLease lease) noexcept -> void {
  assert(state_.load() == State::Created);
  submission_lease_ = std::move(lease);
}

auto Completion::deliver_undelivered() noexcept -> void {
  sink_.deliver(
    MLN_STATUS_NATIVE_ERROR, MLN_COMMAND_DISPOSITION_FAILED, {},
    "the completion result could not be delivered", nullptr, 0, 0, nullptr
  );
}

auto Completion::release() noexcept -> void {
  {
    // The sink's queue reference and unused fallback item go before the
    // lease, so a handle that drains sees the delivery finished.
    auto sink = std::move(sink_);
    sink_ = CompletionSink{};
    fallback_.reset();
    // A queued completion's release runs in dispatch, or is abandoned.
    const auto release_user_data =
      sink.queue_ == nullptr ? sink.descriptor_.release_user_data : nullptr;
    const auto user_data = sink.descriptor_.user_data;
    if (release_user_data != nullptr && !process_exiting()) {
      try {
        release_user_data(user_data);
      } catch (...) {
        // Release callbacks must not unwind through the C boundary.
      }
    }
  }
  submission_lease_.reset();
}

auto make_completion_sink(
  const mln_completion* completion, CompletionSink& sink,
  delivery::ItemPtr& fallback
) -> mln_status {
  auto queue = std::shared_ptr<delivery::DeliveryQueueState>{};
  if (completion == nullptr) {
    set_thread_error("completion must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (completion->size < sizeof(mln_completion)) {
    set_thread_error("mln_completion.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto status = delivery::resolve_registration_queue(
    "completion", completion->queue, completion->callback != nullptr, queue
  );
  if (status != MLN_STATUS_OK) return status;
  try {
    sink.descriptor_ = *completion;
    sink.descriptor_.queue = MLN_HANDLE_NULL;
    if (queue != nullptr) {
      sink.queue_ = std::move(queue);
      const auto size = sizeof(delivery::DeliveryItem) + reserved_result_bytes;
      fallback = delivery::ItemPtr{delivery::detail::allocate_item(size)};
      set_completion_item(*fallback, sink.descriptor_);
      auto arena = delivery::Arena<true>{
        .storage = reinterpret_cast<std::byte*>(fallback.get()),
        .offset = sizeof(delivery::DeliveryItem),
        .capacity = size,
      };
      fallback->arguments = place_result(arena);
      sink.held_ =
        delivery::hold_user_data(sink.queue_, sink.descriptor_.user_data);
    }
  } catch (...) {
    set_thread_error("asynchronous completion state could not be allocated");
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

auto make_completion(
  const mln_completion* descriptor, std::shared_ptr<Completion>& out
) -> mln_status {
  auto sink = CompletionSink{};
  auto fallback = delivery::ItemPtr{};
  const auto status = make_completion_sink(descriptor, sink, fallback);
  if (status != MLN_STATUS_OK) return status;
  try {
    out = std::make_shared<Completion>(std::move(sink), std::move(fallback));
  } catch (...) {
    out.reset();
    set_thread_error("asynchronous completion state could not be allocated");
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

auto make_completion(
  const mln_completion* descriptor, std::optional<Completion>& out
) -> mln_status {
  auto sink = CompletionSink{};
  auto fallback = delivery::ItemPtr{};
  const auto status = make_completion_sink(descriptor, sink, fallback);
  if (status != MLN_STATUS_OK) return status;
  out.emplace(std::move(sink), std::move(fallback));
  return MLN_STATUS_OK;
}

auto make_command_completion(
  const mln_completion* descriptor, std::shared_ptr<Completion>& out
) -> mln_status {
  if (descriptor == nullptr) {
    out = nullptr;
    return MLN_STATUS_OK;
  }
  return make_completion(descriptor, out);
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
  const std::shared_ptr<Completion>& completion,
  mln_command_disposition disposition, mln_status status,
  CommandGenerations generations, std::string diagnostic
) noexcept -> void {
  complete_command(
    completion.get(), disposition, status, generations, std::move(diagnostic)
  );
}

auto complete_command(
  Completion* completion, mln_command_disposition disposition,
  mln_status status, CommandGenerations generations, std::string diagnostic
) noexcept -> void {
  if (status != MLN_STATUS_OK) {
    disposition = status == MLN_STATUS_CANCELLED
                    ? MLN_COMMAND_DISPOSITION_CANCELLED
                    : MLN_COMMAND_DISPOSITION_FAILED;
  }
  if (completion == nullptr) {
    if (disposition == MLN_COMMAND_DISPOSITION_FAILED) {
      try {
        mln::Log::Warning(
          mln::Event::General, "discarded command failed (status " +
                                 std::to_string(status) + "): " + diagnostic
        );
      } catch (...) {
      }
    }
    return;
  }
  completion->resolve([disposition, status, generations,
                       diagnostic =
                         std::move(diagnostic)](const CompletionSink& sink) {
    sink.deliver(
      status, disposition, generations, diagnostic, nullptr, 0, 0, nullptr
    );
  });
}

}  // namespace mln::core
