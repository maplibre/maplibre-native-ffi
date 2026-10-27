// mln::core::Completion, the state machine behind every asynchronous
// completion: delivery once, in order, and only after acceptance.

#include <atomic>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "completion/completion.hpp"

#include "completion/completion_result.hpp"
#include "internal/support/allocation_faults.hpp"
#include "maplibre_native_c.h"
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
  auto view = mln_delivery_batch_view{.size = sizeof(mln_delivery_batch_view)};
  const auto status = mln_delivery_queue_drain(queue, &batch, &view, nullptr);
  if (status == MLN_STATUS_NOT_READY) return {};
  MLN_TEST_OK(status);
  auto drained = QueuedCompletions{.count = view.delivery_count};
  const auto* bytes = static_cast<const unsigned char*>(
    static_cast<const void*>(view.deliveries)
  );
  for (auto index = std::size_t{0}; index < view.delivery_count; ++index) {
    const auto* delivery = reinterpret_cast<const mln_delivery*>(
      bytes + (index * view.delivery_size)
    );
    TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_COMPLETION, delivery->type);
    drained.status = delivery->payload.completion.status;
    drained.user_data = delivery->user_data;
  }
  mln_delivery_batch_release(batch);
  return drained;
}

// Resolves completion with a success that carries no value, as a runtime
// resource configuration command's does.
void succeed(const std::shared_ptr<mln::core::Completion>& completion) {
  mln::core::valueless_completion<&mln_runtime_set_maximum_ambient_cache_size>()
    .complete(completion);
}

// Whether the submission lease that a completion held has been released: a
// closing handle reports drained only once every lease is gone.
auto released_lease(mln::core::ControlState& control) -> bool {
  MLN_TEST_OK(control.begin_close());
  return control.notify_when_drained([](void*) noexcept {}, nullptr);
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
// submission, which reports a native error, and the completion still releases
// its submission lease.
void queue_resolution_falls_back_without_allocation() {
  const auto queue = create_queue();
  auto tag = 0;
  auto control = mln::core::ControlState{};
  TEST_ASSERT_TRUE(control.acquire());
  auto completion = created(queued_descriptor(queue, &tag));
  completion->set_submission_lease(mln::core::ControlLease{&control});
  completion->accept();
  auto text = std::string(1024, 'x');
  {
    const auto faults = mln::native_tests::AllocationFaults{};
    mln::core::CompletionValue<&mln_map_get_loaded_style_json>::complete(
      completion, mln_buffer_view{.data = text.data(), .size = text.size()}
    );
  }
  TEST_ASSERT_TRUE(released_lease(control));
  const auto drained = drain_completions(queue);
  TEST_ASSERT_EQUAL_size_t(1, drained.count);
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, drained.status);
  TEST_ASSERT_EQUAL_PTR(&tag, drained.user_data);
  mln_delivery_queue_release(queue);
}

// A queued readback keeps its image until the batch is released, even when the
// result is small enough for the reserved fallback item, which holds no image.
void a_queued_readback_keeps_its_image_until_the_batch_is_released() {
  const auto queue = create_queue();
  auto completion = created(queued_descriptor(queue, nullptr));
  completion->accept();
  auto image = std::make_shared<std::vector<unsigned char>>(16);
  const auto watched = std::weak_ptr<const void>{image};
  completion->resolve([&image](const mln::core::CompletionSink& sink) {
    const auto value = mln_texture_readback_result{
      .data = {.data = image->data(), .size = image->size()},
    };
    mln::core::CompletionValue<&mln_render_session_read_texture>::deliver(
      sink, value, std::exchange(image, nullptr)
    );
  });
  completion.reset();
  TEST_ASSERT_FALSE(watched.expired());

  auto batch = mln_delivery_batch{MLN_HANDLE_NULL};
  auto view = mln_delivery_batch_view{.size = sizeof(mln_delivery_batch_view)};
  MLN_TEST_OK(mln_delivery_queue_drain(queue, &batch, &view, nullptr));
  mln_delivery_batch_release(batch);
  TEST_ASSERT_TRUE(watched.expired());
  mln_delivery_queue_release(queue);
}

void a_delivery_that_throws_after_the_sink_reports_once() {
  auto probe = CompletionProbe{};
  auto completion = created(descriptor_for(probe));
  completion->accept();
  completion->resolve([](const mln::core::CompletionSink& sink) {
    mln::core::valueless_completion<
      &mln_runtime_set_maximum_ambient_cache_size>()
      .deliver(sink);
    throw std::runtime_error{"delivery failed after the sink"};
  });
  TEST_ASSERT_EQUAL_UINT(1, probe.calls.load());
  TEST_ASSERT_EQUAL_UINT(1, probe.releases.load());
  MLN_TEST_OK(probe.status.load());
}

// An abandoned queue completion pushes one cancellation before it releases its
// submission lease.
void abandoned_queue_completion_pushes_cancelled() {
  const auto queue = create_queue();
  auto control = mln::core::ControlState{};
  TEST_ASSERT_TRUE(control.acquire());
  {
    auto completion = created(queued_descriptor(queue, nullptr));
    completion->set_submission_lease(mln::core::ControlLease{&control});
    completion->accept();
  }
  TEST_ASSERT_TRUE(released_lease(control));
  const auto drained = drain_completions(queue);
  TEST_ASSERT_EQUAL_size_t(1, drained.count);
  MLN_TEST_STATUS(MLN_STATUS_CANCELLED, drained.status);
  mln_delivery_queue_release(queue);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(racing_resolutions_deliver_once);
  RUN_TEST(a_throwing_delivery_reports_a_native_error);
  RUN_TEST(queue_resolution_falls_back_without_allocation);
  RUN_TEST(a_queued_readback_keeps_its_image_until_the_batch_is_released);
  RUN_TEST(a_delivery_that_throws_after_the_sink_reports_once);
  RUN_TEST(abandoned_queue_completion_pushes_cancelled);
}
