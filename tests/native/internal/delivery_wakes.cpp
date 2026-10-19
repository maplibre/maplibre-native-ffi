// When a delivery queue's wake runs: never under MapLibre's global log mutex,
// and again after a drain that failed, so queued deliveries are not stranded.

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <mln/util/event.hpp>
#include <mln/util/logging.hpp>

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

// A wake that logs once. Under the log mutex, its record would lock that
// mutex again on the same thread.
struct LoggingWake {
  mln_delivery_queue queue = MLN_HANDLE_NULL;
  std::atomic_bool started = false;
  std::atomic_bool logged = false;
  std::atomic_bool released = false;
};

void log_from_wake(void* user_data) noexcept {
  auto& wake = *static_cast<LoggingWake*>(user_data);
  if (wake.started.exchange(true)) return;
  mln::Log::Warning(mln::Event::General, "a record from the queue's wake");
  wake.logged.store(true);
  mln_test_pulse();
}

void release_logging_wake(void* user_data) noexcept {
  static_cast<LoggingWake*>(user_data)->released.store(true);
  mln_test_pulse();
}

void a_log_queue_wake_may_log_itself() {
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
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_wait_for_flag(&wake.logged), "the queue's wake never logged"
    );
    MLN_TEST_OK(mln_log_clear_callback(nullptr));

    // Both records, then the handler's retirement.
    auto batch = MLN_HANDLE_NULL;
    MLN_TEST_OK(mln_delivery_queue_drain(wake.queue, &batch, nullptr));
    const auto view = delivery_view(batch);
    TEST_ASSERT_EQUAL_size_t(3, view.delivery_count);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_DELIVERY_TYPE_LOG_RECORD, delivery_type(view, 0)
    );
    TEST_ASSERT_EQUAL_UINT32(
      MLN_DELIVERY_TYPE_LOG_RECORD, delivery_type(view, 1)
    );
    TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_RETIRED, delivery_type(view, 2));
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
    TEST_ASSERT_EQUAL_INT(2, wake.wakes.load());

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

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(a_log_queue_wake_may_log_itself);
  RUN_TEST(a_failed_drain_wakes_the_queue_again);
}
