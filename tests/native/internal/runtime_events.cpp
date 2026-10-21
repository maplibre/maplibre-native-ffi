#include <atomic>
#include <memory>
#include <string>

#include "delivery/delivery_queue.hpp"
#include "internal/support/allocation_faults.hpp"
#include "support/test_support.h"
#include "wake/wake.hpp"

namespace {

void a_failed_event_append_keeps_the_published_events_readable() {
  auto wakes = std::atomic_int{0};
  auto wake = std::make_shared<mln::core::Wake>(mln_wake{
    .callback = [](
                  void* context
                ) { static_cast<std::atomic_int*>(context)->fetch_add(1); },
    .user_data = &wakes,
  });
  wake->accept();
  auto queue = mln::delivery::DeliveryQueueState{wake};
  const auto event =
    mln_runtime_event{.type = MLN_RUNTIME_EVENT_MAP_LOADING_FAILED};
  queue.push_event(event, "first", nullptr);
  const auto message = std::string(65536, 'x');
  auto threw = false;
  {
    const auto faults = mln::native_tests::AllocationFaults{};
    try {
      queue.push_event(event, message, nullptr);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
  }
  auto view = mln_delivery_batch_view{};
  const auto batch = queue.drain(view);
  const auto& events = *view.deliveries[0].payload.runtime_events.events;
  TEST_ASSERT_TRUE(threw);
  TEST_ASSERT_EQUAL_size_t(1, events.event_count);
  TEST_ASSERT_EQUAL_STRING("first", events.events[0].message.data);
  mln::delivery::release_delivery_batch(batch);
  queue.push_event(event, "later", nullptr);
  TEST_ASSERT_EQUAL_INT(2, wakes.load());
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(a_failed_event_append_keeps_the_published_events_readable);
}
