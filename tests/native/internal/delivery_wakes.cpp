// Inline log wakes and deferred recovery after a failed delivery drain.

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <mln/util/event.hpp>
#include <mln/util/logging.hpp>

#include "delivery/delivery_queue.hpp"
#include "maplibre_native_c.h"
#include "support/allocation_faults.hpp"
#include "support/test_support.h"

namespace {

void restore_log_defaults() {
  static_cast<void>(mln_log_clear_callback(nullptr));
  static_cast<void>(
    mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_MASK_DEFAULT, nullptr)
  );
}

// Records logged on the calling thread reach the handler while MapLibre holds
// its log mutex.
void log_synchronously() {
  MLN_TEST_OK(mln_log_set_async_severity_mask(0, nullptr));
}

auto delivery_view(mln_delivery_batch batch) -> mln_delivery_batch_view {
  auto view = mln_delivery_batch_view{.size = sizeof(mln_delivery_batch_view)};
  MLN_TEST_OK(mln_delivery_batch_get(batch, &view, nullptr));
  TEST_ASSERT_TRUE(view.delivery_size >= sizeof(mln_delivery));
  return view;
}

// Steps to one delivery by the stride that native reports.
auto delivery_type(const mln_delivery_batch_view& view, std::size_t index)
  -> std::uint32_t {
  TEST_ASSERT_TRUE(index < view.delivery_count);
  const auto* bytes = static_cast<const unsigned char*>(
    static_cast<const void*>(view.deliveries)
  );
  return reinterpret_cast<const mln_delivery*>(
           bytes + (index * view.delivery_size)
  )
    ->type;
}

auto queue_handler(mln_delivery_queue queue) -> mln_log_handler {
  return mln_log_handler{
    .size = sizeof(mln_log_handler),
    .queue = queue,
  };
}

// A wake that records inline service without reentering the logging API.
struct LoggingWake {
  mln_delivery_queue queue = MLN_HANDLE_NULL;
  std::atomic_bool started = false;
  std::atomic_bool logged = false;
  std::atomic_bool released = false;
};

void log_from_wake(void* user_data) noexcept {
  auto& wake = *static_cast<LoggingWake*>(user_data);
  if (wake.started.exchange(true)) return;
  // Log-handler wakes may only schedule service; record the inline call.
  wake.logged.store(true);
  mln_test_pulse();
}

void release_logging_wake(void* user_data) noexcept {
  static_cast<LoggingWake*>(user_data)->released.store(true);
  mln_test_pulse();
}

void a_log_queue_wakes_inline() {
  if (TEST_PROTECT()) {
    log_synchronously();
    auto wake = LoggingWake{};
    const auto options = mln_delivery_queue_options{
      .size = sizeof(mln_delivery_queue_options),
      .wake = {
        .callback = log_from_wake,
        .user_data = &wake,
        .release_user_data = release_logging_wake,
      },
    };
    MLN_TEST_OK(mln_delivery_queue_create(&options, &wake.queue, nullptr));
    const auto handler = queue_handler(wake.queue);
    MLN_TEST_OK(mln_log_set_callback(&handler, nullptr));

    mln::Log::Warning(mln::Event::General, "a record from the test");
    TEST_ASSERT_TRUE(wake.logged.load());
    MLN_TEST_OK(mln_log_clear_callback(nullptr));

    // The record, then the handler's retirement.
    auto batch = MLN_HANDLE_NULL;
    MLN_TEST_OK(mln_delivery_queue_drain(wake.queue, &batch, nullptr));
    const auto view = delivery_view(batch);
    TEST_ASSERT_EQUAL_size_t(2, view.delivery_count);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_DELIVERY_TYPE_LOG_RECORD, delivery_type(view, 0)
    );
    TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_RETIRED, delivery_type(view, 1));
    mln_delivery_batch_release(batch);

    mln_delivery_queue_release(wake.queue);
    TEST_ASSERT_TRUE(mln_test_wait_for_flag(&wake.released));
  }
  restore_log_defaults();
}

// Counts a queue's wakes, which arrive on native threads.
struct CountingWake {
  mln_delivery_queue queue = MLN_HANDLE_NULL;
  std::atomic_int wakes = 0;
  std::atomic_bool released = false;
};

void count_wake(void* user_data) noexcept {
  static_cast<CountingWake*>(user_data)->wakes.fetch_add(1);
  mln_test_pulse();
}

void release_counting_wake(void* user_data) noexcept {
  static_cast<CountingWake*>(user_data)->released.store(true);
  mln_test_pulse();
}

// A drain that cannot allocate its batch leaves the deliveries queued and
// wakes the receiver again, so a receiver that waits for the next wake still
// drains them.
void a_failed_drain_wakes_the_queue_again() {
  if (TEST_PROTECT()) {
    log_synchronously();
    auto wake = CountingWake{};
    const auto options = mln_delivery_queue_options{
      .size = sizeof(mln_delivery_queue_options),
      .wake = {
        .callback = count_wake,
        .user_data = &wake,
        .release_user_data = release_counting_wake,
      },
    };
    MLN_TEST_OK(mln_delivery_queue_create(&options, &wake.queue, nullptr));
    const auto handler = queue_handler(wake.queue);
    MLN_TEST_OK(mln_log_set_callback(&handler, nullptr));
    mln::Log::Warning(mln::Event::General, "a queued record");
    TEST_ASSERT_TRUE(mln_test_wait_for_count(&wake.wakes, 1));

    auto failed = MLN_STATUS_OK;
    {
      const auto faults = mln::native_tests::AllocationFaults{};
      TEST_ASSERT_TRUE_MESSAGE(
        mln::native_tests::AllocationFaults::active(),
        "allocation fault injection is inactive"
      );
      auto batch = MLN_HANDLE_NULL;
      failed = mln_delivery_queue_drain(wake.queue, &batch, nullptr);
    }
    MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, failed);
    TEST_ASSERT_TRUE(mln_test_wait_for_count(&wake.wakes, 2));

    auto batch = MLN_HANDLE_NULL;
    MLN_TEST_OK(mln_delivery_queue_drain(wake.queue, &batch, nullptr));
    const auto view = delivery_view(batch);
    TEST_ASSERT_EQUAL_size_t(1, view.delivery_count);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_DELIVERY_TYPE_LOG_RECORD, delivery_type(view, 0)
    );
    mln_delivery_batch_release(batch);

    MLN_TEST_OK(mln_log_clear_callback(nullptr));
    mln_delivery_queue_release(wake.queue);
    TEST_ASSERT_TRUE(mln_test_wait_for_flag(&wake.released));
  }
  restore_log_defaults();
}

void failed_batch_reservations_retain_the_queued_prefix() {
  for (auto budget = std::size_t{}; budget < 5; ++budget) {
    const auto options = mln_delivery_queue_options{
      .size = sizeof(mln_delivery_queue_options),
    };
    auto handle = mln_delivery_queue{MLN_HANDLE_NULL};
    MLN_TEST_OK(mln_delivery_queue_create(&options, &handle, nullptr));
    auto queue = mln::delivery::resolve_delivery_queue(handle);
    auto item =
      mln::delivery::make_light_item(MLN_DELIVERY_TYPE_RETIRED, nullptr);
    const auto pushed = queue->push(item);
    auto batch = MLN_HANDLE_NULL;
    auto status = MLN_STATUS_OK;
    {
      const auto faults = mln::native_tests::AllocationFaults{budget};
      status = mln_delivery_queue_drain(handle, &batch, nullptr);
    }
    if (status == MLN_STATUS_NATIVE_ERROR) {
      status = mln_delivery_queue_drain(handle, &batch, nullptr);
    }
    auto view =
      mln_delivery_batch_view{.size = sizeof(mln_delivery_batch_view)};
    const auto viewed = mln_delivery_batch_get(batch, &view, nullptr);
    const auto count = view.delivery_count;
    mln_delivery_batch_release(batch);
    mln_delivery_queue_release(handle);
    TEST_ASSERT_TRUE(pushed);
    MLN_TEST_OK(status);
    MLN_TEST_OK(viewed);
    TEST_ASSERT_EQUAL_size_t(1, count);
  }
}

struct InlineDrain {
  mln_delivery_queue queue = MLN_HANDLE_NULL;
  std::atomic_int depth = 0;
  std::atomic_int maximum_depth = 0;
  std::atomic_int calls = 0;
  std::atomic_int status = MLN_STATUS_OK;
  std::atomic_bool released = false;
};

void drain_without_memory(void* context) noexcept {
  auto& probe = *static_cast<InlineDrain*>(context);
  const auto depth = probe.depth.fetch_add(1) + 1;
  if (depth > probe.maximum_depth.load()) probe.maximum_depth = depth;
  if (depth == 1) {
    const auto faults = mln::native_tests::AllocationFaults{};
    auto batch = MLN_HANDLE_NULL;
    probe.status = mln_delivery_queue_drain(probe.queue, &batch, nullptr);
  }
  --probe.depth;
  ++probe.calls;
  mln_test_pulse();
}

void failed_inline_drains_get_one_deferred_retry_without_reentry() {
  auto probe = InlineDrain{};
  const auto options = mln_delivery_queue_options{
    .size = sizeof(mln_delivery_queue_options),
    .wake = {
      .callback = drain_without_memory,
      .user_data = &probe,
      .release_user_data = [](void* context) noexcept {
        static_cast<InlineDrain*>(context)->released = true;
        mln_test_pulse();
      },
    },
  };
  MLN_TEST_OK(mln_delivery_queue_create(&options, &probe.queue, nullptr));
  auto queue = mln::delivery::resolve_delivery_queue(probe.queue);
  auto item =
    mln::delivery::make_light_item(MLN_DELIVERY_TYPE_RETIRED, nullptr);
  const auto pushed = queue->push(item);
  const auto retried = mln_test_wait_for_count(&probe.calls, 2);
  // Both wakes fail; a host-scheduled drain recovers the retained item.
  auto batch = MLN_HANDLE_NULL;
  const auto recovered = mln_delivery_queue_drain(probe.queue, &batch, nullptr);
  auto view = mln_delivery_batch_view{.size = sizeof(mln_delivery_batch_view)};
  const auto viewed = mln_delivery_batch_get(batch, &view, nullptr);
  const auto count = view.delivery_count;
  mln_delivery_batch_release(batch);
  mln_delivery_queue_release(probe.queue);
  const auto released = mln_test_wait_for_flag(&probe.released);
  TEST_ASSERT_TRUE(pushed);
  TEST_ASSERT_TRUE(retried);
  TEST_ASSERT_TRUE(released);
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, probe.status.load());
  MLN_TEST_OK(recovered);
  MLN_TEST_OK(viewed);
  TEST_ASSERT_EQUAL_size_t(1, count);
  TEST_ASSERT_EQUAL_INT(1, probe.maximum_depth.load());
  TEST_ASSERT_EQUAL_INT(2, probe.calls.load());
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(a_log_queue_wakes_inline);
  RUN_TEST(failed_inline_drains_get_one_deferred_retry_without_reentry);
  RUN_TEST(a_failed_drain_wakes_the_queue_again);
  RUN_TEST(failed_batch_reservations_retain_the_queued_prefix);
}
