// mln::core::Completion, the state machine behind every asynchronous
// completion: delivery once, in order, and only after acceptance.

#include <atomic>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <thread>

#include "completion/completion.hpp"

#include "completion/completion_result.hpp"
#include "internal/support/allocation_faults.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "support/harness.h"
#include "support/status.h"
#include "support/test_support.h"
#include "unity.h"

namespace {

// Counts what one completion descriptor did, and in which order.
struct CompletionProbe {
  std::atomic_uint calls = 0;
  std::atomic_uint releases = 0;
  // 0 before delivery, 1 after the callback, 2 after the release, -1 once
  // either arrived out of order.
  std::atomic_int phase = 0;
  std::atomic_int status = MLN_STATUS_INVALID_STATE;
  std::thread::id thread;
};

void record_completion(
  void* user_data, const mln_completion_result* result
) noexcept {
  auto& probe = *static_cast<CompletionProbe*>(user_data);
  auto expected = 0;
  if (!probe.phase.compare_exchange_strong(expected, 1)) {
    probe.phase.store(-1);
  }
  probe.thread = std::this_thread::get_id();
  probe.status.store(result->status);
  probe.calls.fetch_add(1);
}

void record_release(void* user_data) noexcept {
  auto& probe = *static_cast<CompletionProbe*>(user_data);
  auto expected = 1;
  if (!probe.phase.compare_exchange_strong(expected, 2)) {
    probe.phase.store(-1);
  }
  probe.releases.fetch_add(1);
}

auto descriptor_for(CompletionProbe& probe) -> mln_completion {
  return mln_completion{
    .size = sizeof(mln_completion),
    .callback = record_completion,
    .user_data = &probe,
    .release_user_data = record_release,
  };
}

auto created(const mln_completion& descriptor)
  -> std::shared_ptr<mln::core::Completion> {
  auto completion = std::shared_ptr<mln::core::Completion>{};
  MLN_TEST_OK(mln::core::make_completion(&descriptor, completion));
  return completion;
}

auto create_queue() -> mln_delivery_queue {
  const auto options =
    mln_delivery_queue_options{.size = sizeof(mln_delivery_queue_options)};
  auto queue = mln_delivery_queue{MLN_HANDLE_NULL};
  MLN_TEST_OK(mln_delivery_queue_create(&options, &queue, nullptr));
  return queue;
}

auto queued_descriptor(mln_delivery_queue queue, void* user_data)
  -> mln_completion {
  return mln_completion{
    .size = sizeof(mln_completion),
    .callback = nullptr,
    .user_data = user_data,
    .release_user_data = nullptr,
    .queue = queue,
  };
}

// The completions that one drain of queue holds.
struct QueuedCompletions {
  std::size_t count = 0;
  mln_status status = MLN_STATUS_INVALID_STATE;
  void* user_data = nullptr;
};

auto drain_completions(mln_delivery_queue queue) -> QueuedCompletions {
  auto batch = mln_delivery_batch{MLN_HANDLE_NULL};
  const auto status = mln_delivery_queue_drain(queue, &batch, nullptr);
  if (status == MLN_STATUS_NOT_READY) return {};
  MLN_TEST_OK(status);
  auto view = mln_delivery_batch_view{.size = sizeof(mln_delivery_batch_view)};
  MLN_TEST_OK(mln_delivery_batch_get(batch, &view, nullptr));
  auto drained = QueuedCompletions{.count = view.delivery_count};
  const auto* bytes = static_cast<const unsigned char*>(
    static_cast<const void*>(view.deliveries)
  );
  for (auto index = std::size_t{0}; index < view.delivery_count; ++index) {
    const auto* delivery = reinterpret_cast<const mln_delivery*>(
      bytes + (index * view.delivery_size)
    );
    TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_COMPLETION, delivery->type);
    drained.status =
      static_cast<mln_status>(delivery->payload.completion.status);
    drained.user_data = delivery->user_data;
  }
  mln_delivery_batch_release(batch);
  return drained;
}

// Resolves completion with a success that carries no value, as the runtime
// barrier's does.
void succeed(const std::shared_ptr<mln::core::Completion>& completion) {
  mln::core::valueless_completion<&mln_runtime_barrier>().complete(completion);
}

void accepted_delivery_stays_on_the_resolving_thread() {
  auto probe = CompletionProbe{};
  auto completion = created(descriptor_for(probe));
  completion->accept();
  auto worker = std::thread{[completion]() { succeed(completion); }};
  const auto worker_id = worker.get_id();
  worker.join();
  completion->accept();
  completion->resolve([](const mln::core::CompletionSink&) {});
  TEST_ASSERT_TRUE(probe.thread == worker_id);
  TEST_ASSERT_EQUAL_UINT(1, probe.calls.load());
  TEST_ASSERT_EQUAL_UINT(1, probe.releases.load());
  TEST_ASSERT_EQUAL_INT(2, probe.phase.load());
  MLN_TEST_OK(probe.status.load());
}

void revoked_acceptance_leaves_callback_state_with_the_caller() {
  auto probe = CompletionProbe{};
  {
    auto completion = created(descriptor_for(probe));
    completion->accept();
    completion->revoke();
    succeed(completion);
  }
  TEST_ASSERT_EQUAL_UINT(0, probe.calls.load());
  TEST_ASSERT_EQUAL_UINT(0, probe.releases.load());
}

// A rejected completion leaves the user data with the caller and delivers
// nothing.
void rejection_leaves_the_user_data() {
  auto probe = CompletionProbe{};
  {
    auto completion = created(descriptor_for(probe));
    completion->reject();
  }
  TEST_ASSERT_EQUAL_UINT(0, probe.calls.load());
  TEST_ASSERT_EQUAL_UINT(0, probe.releases.load());
}

// An accepted completion nothing resolves reports MLN_STATUS_CANCELLED when it
// is destroyed.
void abandonment_reports_cancelled() {
  auto probe = CompletionProbe{};
  {
    auto completion = created(descriptor_for(probe));
    completion->accept();
  }
  TEST_ASSERT_EQUAL_UINT(1, probe.calls.load());
  TEST_ASSERT_EQUAL_UINT(1, probe.releases.load());
  MLN_TEST_STATUS(MLN_STATUS_CANCELLED, probe.status.load());
}

// Two terminal paths racing still deliver once, on one of their workers.
void racing_resolutions_deliver_once() {
  for (auto iteration = 0; iteration < 100; ++iteration) {
    auto probe = CompletionProbe{};
    auto completion = created(descriptor_for(probe));
    completion->accept();
    auto first = std::thread{[completion]() { succeed(completion); }};
    auto second = std::thread{[completion]() { succeed(completion); }};
    first.join();
    second.join();
    TEST_ASSERT_TRUE(probe.thread != std::this_thread::get_id());
    TEST_ASSERT_EQUAL_UINT(1, probe.calls.load());
    TEST_ASSERT_EQUAL_UINT(1, probe.releases.load());
    TEST_ASSERT_EQUAL_INT(2, probe.phase.load());
  }
}

void terminal_observation_waits_for_result_delivery() {
  auto delivery = mln_test_gate{};
  mln_test_gate_init(&delivery);
  auto observed = std::atomic_bool{false};
  auto operation = std::make_shared<mln::core::OperationObject>(
    [&delivery](mln_status, std::string, std::any) {
      mln_test_gate_park(&delivery);
    }
  );
  auto resolver =
    std::thread{[operation] { operation->complete(MLN_STATUS_OK, {}, {}); }};
  const auto delivering = mln_test_gate_wait_entered(&delivery);
  operation->set_terminal_callback([&observed] { observed = true; });
  const auto observed_early = observed.load();
  mln_test_gate_release(&delivery);
  resolver.join();
  TEST_ASSERT_TRUE(delivering);
  TEST_ASSERT_FALSE(observed_early);
  TEST_ASSERT_TRUE(observed.load());
}

void accepted_completion_resolution_needs_no_allocation() {
  for (auto kind = 0; kind != 3; ++kind) {
    auto probe = CompletionProbe{};
    auto completion = created(descriptor_for(probe));
    auto diagnostic = std::string(128, 'x');
    completion->accept();
    {
      const auto faults = mln::native_tests::AllocationFaults{};
      if (kind == 0)
        succeed(completion);
      else if (kind == 1)
        mln::core::CompletionValue<&mln_map_get_camera>::complete(
          completion, {}
        );
      else
        mln::core::complete_failure(
          completion, MLN_STATUS_NATIVE_ERROR, std::move(diagnostic)
        );
    }
    TEST_ASSERT_EQUAL_UINT(1, probe.calls.load());
    TEST_ASSERT_EQUAL_UINT(1, probe.releases.load());
    MLN_TEST_STATUS(
      kind == 2 ? MLN_STATUS_NATIVE_ERROR : MLN_STATUS_OK, probe.status.load()
    );
  }
}

// A delivery that throws before it reaches the sink still reports one result,
// a native error, and then releases the user data.
void a_throwing_delivery_reports_a_native_error() {
  auto probe = CompletionProbe{};
  auto completion = created(descriptor_for(probe));
  completion->accept();
  completion->resolve([](const mln::core::CompletionSink&) {
    throw std::runtime_error{"delivery failed before the sink"};
  });
  TEST_ASSERT_EQUAL_UINT(1, probe.calls.load());
  TEST_ASSERT_EQUAL_UINT(1, probe.releases.load());
  TEST_ASSERT_EQUAL_INT(2, probe.phase.load());
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, probe.status.load());
}

// A queue item that cannot be allocated falls back to the one reserved at
// submission, which reports a native error, and the completion still retires.
void queue_resolution_falls_back_without_allocation() {
  const auto queue = create_queue();
  auto tag = 0;
  auto retired = false;
  auto completion = created(queued_descriptor(queue, &tag));
  completion->set_retirement_callback([&retired] { retired = true; });
  completion->accept();
  completion->retire_work();
  {
    const auto faults = mln::native_tests::AllocationFaults{};
    succeed(completion);
  }
  TEST_ASSERT_TRUE(retired);
  const auto drained = drain_completions(queue);
  TEST_ASSERT_EQUAL_size_t(1, drained.count);
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, drained.status);
  TEST_ASSERT_EQUAL_PTR(&tag, drained.user_data);
  mln_delivery_queue_release(queue);
}

// A revoked queue completion pushes nothing.
void revoked_queue_completion_pushes_nothing() {
  const auto queue = create_queue();
  {
    auto completion = created(queued_descriptor(queue, nullptr));
    completion->accept();
    completion->revoke();
    succeed(completion);
  }
  TEST_ASSERT_EQUAL_size_t(0, drain_completions(queue).count);
  mln_delivery_queue_release(queue);
}

// An abandoned queue completion pushes one cancellation before it retires.
void abandoned_queue_completion_pushes_cancelled() {
  const auto queue = create_queue();
  auto retired = false;
  {
    auto completion = created(queued_descriptor(queue, nullptr));
    completion->set_retirement_callback([&retired] { retired = true; });
    completion->accept();
    completion->retire_work();
  }
  TEST_ASSERT_TRUE(retired);
  const auto drained = drain_completions(queue);
  TEST_ASSERT_EQUAL_size_t(1, drained.count);
  MLN_TEST_STATUS(MLN_STATUS_CANCELLED, drained.status);
  mln_delivery_queue_release(queue);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(terminal_observation_waits_for_result_delivery);
  RUN_TEST(accepted_completion_resolution_needs_no_allocation);

  RUN_TEST(accepted_delivery_stays_on_the_resolving_thread);
  RUN_TEST(revoked_acceptance_leaves_callback_state_with_the_caller);
  RUN_TEST(rejection_leaves_the_user_data);
  RUN_TEST(abandonment_reports_cancelled);
  RUN_TEST(racing_resolutions_deliver_once);
  RUN_TEST(a_throwing_delivery_reports_a_native_error);
  RUN_TEST(queue_resolution_falls_back_without_allocation);
  RUN_TEST(revoked_queue_completion_pushes_nothing);
  RUN_TEST(abandoned_queue_completion_pushes_cancelled);
}
