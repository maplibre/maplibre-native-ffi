// A log handler's platform severities choose the records that MapLibre also
// prints to the platform's log, with or without a delivery queue. MapLibre
// prints a record when its log observer returns false, so each case takes
// the handler's observer and asks it about one record of each severity.

#include <cstdint>
#include <memory>
#include <utility>

#include <mln/util/event.hpp>
#include <mln/util/logging.hpp>

#include "maplibre_native_c.h"
#include "support/delivery.h"
#include "support/test_support.h"

namespace {

void count_record(
  void* user_data, mln_log_severity severity, mln_log_event event,
  std::int64_t code, mln_buffer_view message
) {
  static_cast<void>(severity);
  static_cast<void>(event);
  static_cast<void>(code);
  static_cast<void>(message);
  ++*static_cast<int*>(user_data);
}

constexpr std::pair<mln::EventSeverity, mln_log_severity_flag> severities[] = {
  {mln::EventSeverity::Info, MLN_LOG_SEVERITY_FLAG_INFO},
  {mln::EventSeverity::Warning, MLN_LOG_SEVERITY_FLAG_WARNING},
  {mln::EventSeverity::Error, MLN_LOG_SEVERITY_FLAG_ERROR},
};

constexpr mln_log_severity_flags platform_masks[] = {
  0,
  MLN_LOG_SEVERITY_FLAG_WARNING | MLN_LOG_SEVERITY_FLAG_ERROR,
  MLN_LOG_SEVERITY_FLAG_ALL,
};

// Installs handler and takes its observer back from MapLibre.
auto take_observer(const mln_log_handler& handler)
  -> std::unique_ptr<mln::Log::Observer> {
  MLN_TEST_OK(mln_log_set_callback(&handler, nullptr));
  auto observer = mln::Log::removeObserver();
  TEST_ASSERT_NOT_NULL(observer.get());
  return observer;
}

// The severities whose records observer leaves to the platform's log.
auto printed_records(mln::Log::Observer& observer) -> mln_log_severity_flags {
  auto printed = mln_log_severity_flags{0};
  for (const auto& [severity, flag] : severities) {
    if (!observer.onRecord(severity, mln::Event::General, -1, "record")) {
      printed |= flag;
    }
  }
  return printed;
}

auto platform_records(const mln_log_handler& handler)
  -> mln_log_severity_flags {
  return printed_records(*take_observer(handler));
}

void platform_severities_choose_the_printed_records() {
  for (const auto mask : platform_masks) {
    auto records = 0;
    auto handler = mln_log_handler{};
    handler.size = sizeof(mln_log_handler);
    handler.callback = count_record;
    handler.user_data = &records;
    handler.platform_severities = mask;
    TEST_ASSERT_EQUAL_UINT32(mask, platform_records(handler));
    TEST_ASSERT_EQUAL_INT(3, records);
  }
}

void a_queued_handler_applies_its_platform_severities() {
  for (const auto mask : platform_masks) {
    auto queue = mln_test_delivery_queue{};
    mln_test_delivery_queue_create(&queue);
    auto records = 0;
    auto handler = mln_log_handler{};
    handler.size = sizeof(mln_log_handler);
    handler.callback = count_record;
    handler.user_data = &records;
    handler.queue = queue.handle;
    handler.platform_severities = mask;
    TEST_ASSERT_EQUAL_UINT32(mask, platform_records(handler));
    MLN_TEST_OK(mln_delivery_queue_dispatch(queue.handle, nullptr));
    TEST_ASSERT_EQUAL_INT(3, records);
    mln_test_delivery_queue_release(&queue);
  }
}

// A queue that refuses a record, here because the host released it, drops
// the record without changing whether the platform's log gets it.
void a_refused_record_keeps_its_platform_verdict() {
  for (const auto mask : platform_masks) {
    auto queue = mln_test_delivery_queue{};
    mln_test_delivery_queue_create(&queue);
    auto records = 0;
    auto handler = mln_log_handler{};
    handler.size = sizeof(mln_log_handler);
    handler.callback = count_record;
    handler.user_data = &records;
    handler.queue = queue.handle;
    handler.platform_severities = mask;
    auto observer = take_observer(handler);
    mln_delivery_queue_release(queue.handle);
    TEST_ASSERT_EQUAL_UINT32(mask, printed_records(*observer));
    // The observer holds the last share of the queue, so its wake is
    // released with it.
    observer.reset();
    TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue.wake_releases, 1));
    TEST_ASSERT_EQUAL_INT(0, records);
  }
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(platform_severities_choose_the_printed_records);
  RUN_TEST(a_queued_handler_applies_its_platform_severities);
  RUN_TEST(a_refused_record_keeps_its_platform_verdict);
}
